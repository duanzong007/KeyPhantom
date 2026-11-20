#include "mainwindow.h"
#include "platforminput.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QStyle>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->speedCombo->addItems({tr("慢速"), tr("正常"), tr("快速"), tr("超高速")});
    ui->newlineCombo->addItems({tr("Enter"), tr("Shift + Enter")});
    ui->tabCombo->addItems({tr("Tab 按键"), tr("转换为空格")});
    ui->speedCombo->setCurrentIndex(InputOptions::Normal);
    ui->textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->textEdit->setTabStopDistance(ui->textEdit->fontMetrics().horizontalAdvance(u' ') * 4);
    ui->textEdit->setAccessibleName(tr("待输入的文本"));
    ui->progressBar->setAccessibleName(tr("发送进度"));
    ui->stopButton->setToolTip(tr("全局停止：Ctrl + Alt + F8（Mac 使用 Control + Option + F8）"));
    ui->hotkeyLabel->setToolTip(tr("在任何应用中按下即可停止。Mac 如将 F8 用作媒体键，需同时按 Fn。"));
    if (!stopHotkey_.isRegistered()) {
        ui->hotkeyLabel->setText(tr("全局停止不可用 · 请检查快捷键冲突"));
        ui->hotkeyLabel->setProperty("warning", true);
    }
    if (screen()) {
        const QSize available = screen()->availableGeometry().size() - QSize(60, 80);
        resize(size().boundedTo(available).expandedTo(minimumSize()));
    }
    restoreSettings();

    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::startOrResume);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::stopSending);
    connect(&stopHotkey_, &GlobalStopHotkey::activated, this, &MainWindow::stopSending);
    connect(ui->settingsButton, &QPushButton::clicked, this, [this] {
        const bool opening = ui->pageStack->currentWidget() == ui->homePage;
        ui->pageStack->setCurrentWidget(opening ? ui->settingsPage : ui->homePage);
        ui->titleLabel->setText(opening ? tr("设置") : tr("键影"));
        ui->appNameLabel->setVisible(!opening);
        ui->settingsButton->setText(opening ? tr("返回") : tr("设置"));
    });
    connect(ui->permissionButton, &QPushButton::clicked, this, [this] {
        PlatformInput::requestPermission();
#ifdef Q_OS_MACOS
        QDesktopServices::openUrl(QUrl(QStringLiteral("x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility")));
#endif
        refreshPermission();
    });

    previewTimer_.setSingleShot(true);
    previewTimer_.setInterval(100);
    connect(ui->textEdit, &QPlainTextEdit::textChanged, &previewTimer_, qOverload<>(&QTimer::start));
    connect(&previewTimer_, &QTimer::timeout, this, &MainWindow::refreshPlan);
    const auto settingsChanged = [this] {
        refreshPlan();
        saveSettings();
    };
    connect(ui->speedCombo, &QComboBox::currentIndexChanged, this, settingsChanged);
    connect(ui->newlineCombo, &QComboBox::currentIndexChanged, this, settingsChanged);
    connect(ui->tabCombo, &QComboBox::currentIndexChanged, this, settingsChanged);
    connect(ui->delaySpin, &QSpinBox::valueChanged, this, settingsChanged);
    connect(ui->tabWidthSpin, &QSpinBox::valueChanged, this, settingsChanged);
    connect(ui->notifyCheck, &QCheckBox::toggled, this, [this] { saveSettings(); });

    countdownTimer_.setInterval(50);
    connect(&countdownTimer_, &QTimer::timeout, this, &MainWindow::tickCountdown);
    permissionTimer_.setInterval(1000);
    connect(&permissionTimer_, &QTimer::timeout, this, &MainWindow::refreshPermission);
    permissionTimer_.start();
    connect(&sender_, &KeySenderThread::progress, this, &MainWindow::updateProgress);
    connect(&sender_, &KeySenderThread::pauseAcknowledged, this, [this] {
        if (state_ == Paused)
            ui->stateLabel->setText(tr("已暂停"));
    });
    connect(&sender_, &QThread::finished, this, &MainWindow::sendingFinished);
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
#ifdef Q_OS_MACOS
            if (isHidden()) {
                showNormal();
                raise();
                activateWindow();
            }
#endif
            if (state_ == Sending) {
                // The click that activates this window must not also resume the
                // just-paused job through the contextual primary button.
                activationPause_ = true;
                pauseSending();
                setMessage(tr("你已返回键影，输入已暂停。继续后会重新倒计时，请切回目标窗口。"));
                QTimer::singleShot(250, this, [this] {
                    activationPause_ = false;
                    updateControls();
                });
            }
            refreshPermission();
        }
    });
    auto *stopShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(stopShortcut, &QShortcut::activated, this, &MainWindow::stopSending);
#ifdef Q_OS_MACOS
    auto *closeShortcut = new QShortcut(QKeySequence::Close, this);
    connect(closeShortcut, &QShortcut::activated, this, &QWidget::hide);
#endif
    refreshPlan();
    refreshPermission();
    ensurePolished();
    ui->mainLayout->activate();
    setMinimumSize(minimumSize().expandedTo(minimumSizeHint()));
}

MainWindow::~MainWindow()
{
    countdownTimer_.stop();
    sender_.requestStop();
    sender_.wait();
    saveSettings();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    countdownTimer_.stop();
    sender_.requestStop();
    saveSettings();
    QMainWindow::closeEvent(event);
}

InputOptions MainWindow::currentOptions() const
{
    InputOptions options;
    options.speed = static_cast<InputOptions::Speed>(ui->speedCombo->currentIndex());
    options.newlineMode = static_cast<InputOptions::NewlineMode>(ui->newlineCombo->currentIndex());
    options.tabMode = static_cast<InputOptions::TabMode>(ui->tabCombo->currentIndex());
    options.tabWidth = ui->tabWidthSpin->value();
    return options;
}

void MainWindow::restoreSettings()
{
    QSettings settings;
    ui->speedCombo->setCurrentIndex(qBound(0, settings.value("input/speed", 1).toInt(), 3));
    ui->delaySpin->setValue(qBound(1, settings.value("input/delay", 3).toInt(), 30));
    ui->newlineCombo->setCurrentIndex(qBound(0, settings.value("input/newline", 0).toInt(), 1));
    ui->tabCombo->setCurrentIndex(qBound(0, settings.value("input/tab", 0).toInt(), 1));
    ui->tabWidthSpin->setValue(qBound(1, settings.value("input/tabWidth", 4).toInt(), 8));
    ui->notifyCheck->setChecked(settings.value("ui/notify", false).toBool());
    const QByteArray geometry = settings.value("ui/geometry").toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.setValue("input/speed", ui->speedCombo->currentIndex());
    settings.setValue("input/delay", ui->delaySpin->value());
    settings.setValue("input/newline", ui->newlineCombo->currentIndex());
    settings.setValue("input/tab", ui->tabCombo->currentIndex());
    settings.setValue("input/tabWidth", ui->tabWidthSpin->value());
    settings.setValue("ui/notify", ui->notifyCheck->isChecked());
    settings.setValue("ui/geometry", saveGeometry());
}

QString MainWindow::duration(qint64 milliseconds)
{
    const qint64 seconds = (milliseconds + 999) / 1000;
    if (seconds < 60)
        return tr("%1 秒").arg(seconds);
    if (seconds < 3600)
        return tr("%1 分 %2 秒").arg(seconds / 60).arg(seconds % 60);
    return tr("%1 小时 %2 分").arg(seconds / 3600).arg(seconds / 60 % 60);
}

void MainWindow::refreshPlan()
{
    if (state_ != Idle)
        return;
    const QString text = ui->textEdit->toPlainText();
    previewPlan_ = InputPlan::fromText(text, currentOptions());
    const qsizetype lines = text.isEmpty() ? 0 : text.count(u'\n') + 1;
    ui->countLabel->setText(tr("%1 字符 · %2 行").arg(previewPlan_.units.size()).arg(lines));
    ui->estimateLabel->setText(previewPlan_.units.isEmpty() ? tr("预计用时 —")
        : tr("预计用时 %1").arg(duration(previewPlan_.remainingMs(0, currentOptions().speed))));
    updateProgress(0, previewPlan_.units.size(), previewPlan_.remainingMs(0, currentOptions().speed));
    ui->stateLabel->setText(tr("准备就绪"));
    if (permissionGranted_)
        setMessage({});
    updateControls();
}

void MainWindow::refreshPermission()
{
    const bool granted = PlatformInput::hasPermission();
    ui->permissionButton->setVisible(!granted);
    if (!granted && (state_ == Idle || state_ == Paused))
        setMessage(tr("需要辅助功能权限，请在设置中开启。"), true);
    else if (granted && !permissionGranted_)
        setMessage({});
#ifdef Q_OS_MACOS
    ui->permissionStatusLabel->setText(granted ? tr("辅助功能已授权，可以开始输入。")
        : tr("请允许键影使用辅助功能，以便向其他应用输入文字。"));
#else
    ui->permissionStatusLabel->setText(tr("可以使用系统键盘输入。目标应用权限较高时，系统可能阻止输入。"));
#endif
    permissionGranted_ = granted;
    updateControls();
}

void MainWindow::setMessage(const QString &message, bool error)
{
    ui->messageLabel->setText(message);
    ui->messageLabel->setVisible(!message.isEmpty());
    ui->messageLabel->setProperty("error", error);
    ui->messageLabel->style()->unpolish(ui->messageLabel);
    ui->messageLabel->style()->polish(ui->messageLabel);
}

void MainWindow::setState(State state)
{
    state_ = state;
    updateControls();
}

void MainWindow::updateControls()
{
    const bool idle = state_ == Idle;
    ui->textEdit->setReadOnly(!idle);
    ui->settingsCard->setEnabled(idle);
    ui->settingsButton->setEnabled(idle);
    const bool spaces = ui->tabCombo->currentIndex() == InputOptions::Spaces;
    ui->tabWidthSpin->setVisible(spaces);
    ui->tabWidthLabel->setVisible(spaces);
    ui->progressGroup->setVisible(!idle);
    const bool canStart = permissionGranted_ && !ui->textEdit->toPlainText().isEmpty();
    ui->startButton->setEnabled(state_ == Sending || state_ == Countdown
        || (state_ == Paused && permissionGranted_ && !activationPause_) || (idle && canStart));
    ui->startButton->setText(state_ == Paused ? tr("继续输入") : state_ == Countdown ? tr("取消倒计时")
        : state_ == Sending ? tr("暂停输入") : state_ == Stopping ? tr("正在停止…") : tr("开始输入"));
    ui->stopButton->setVisible(state_ == Sending || state_ == Paused || state_ == Stopping
                               || (state_ == Countdown && resuming_));
    ui->stopButton->setEnabled(state_ != Stopping);
}

void MainWindow::startOrResume()
{
    if (state_ == Sending) {
        pauseSending();
        return;
    }
    if (state_ == Countdown) {
        if (resuming_) {
            countdownTimer_.stop();
            setState(Paused);
            ui->stateLabel->setText(tr("已暂停"));
            setMessage(tr("继续倒计时已取消，输入保持暂停。"));
        } else {
            stopSending();
        }
        return;
    }
    if (state_ != Idle && state_ != Paused)
        return;
    if (state_ == Paused && activationPause_)
        return;
    refreshPermission();
    if (!permissionGranted_)
        return;
    if (state_ == Paused) {
        beginCountdown(true);
        return;
    }
    previewTimer_.stop();
    refreshPlan();
    if (previewPlan_.units.isEmpty()) {
        setMessage(tr("请先填入需要输入的文本。"), true);
        return;
    }
    jobPlan_ = previewPlan_;
    jobOptions_ = currentOptions();
    if (!sender_.configure(jobPlan_, jobOptions_)) {
        setMessage(tr("上一次输入尚未结束，请稍后重试。"), true);
        return;
    }
    beginCountdown(false);
}

void MainWindow::beginCountdown(bool resume)
{
    resuming_ = resume;
    countdownMs_ = ui->delaySpin->value() * 1000;
    countdownClock_.start();
    setState(Countdown);
    setMessage(stopHotkey_.isRegistered()
        ? tr("请切换到目标输入框。Ctrl + Alt + F8 可随时停止。")
        : tr("请切换到目标输入框。全局快捷键不可用，返回键影可暂停或停止。"), !stopHotkey_.isRegistered());
    countdownTimer_.start();
    tickCountdown();
}

void MainWindow::tickCountdown()
{
    if (state_ != Countdown)
        return;
    const qint64 remaining = countdownMs_ - countdownClock_.elapsed();
    if (remaining > 0) {
        const qint64 seconds = (remaining + 999) / 1000;
        ui->stateLabel->setText(tr("%1 秒后%2").arg(seconds).arg(resuming_ ? tr("继续") : tr("开始")));
        ui->startButton->setText(tr("取消 · %1 秒").arg(seconds));
        return;
    }
    countdownTimer_.stop();
    if (!PlatformInput::hasPermission() || isActiveWindow()
        || qApp->applicationState() == Qt::ApplicationActive) {
        setState(resuming_ ? Paused : Idle);
        ui->stateLabel->setText(resuming_ ? tr("已暂停") : tr("已取消"));
        setMessage(!PlatformInput::hasPermission()
            ? tr("辅助功能权限未开启，无法开始输入。")
            : tr("尚未切换到目标窗口。请再次开始，并在倒计时内切换到目标输入框。"), true);
        refreshPermission();
        return;
    }
    setState(Sending);
    ui->stateLabel->setText(tr("正在输入"));
    setMessage(stopHotkey_.isRegistered() ? tr("Ctrl + Alt + F8 停止 · 返回键影会自动暂停")
                                           : tr("全局快捷键不可用 · 返回键影可暂停或停止"));
    if (resuming_)
        sender_.setPaused(false);
    else
        sender_.start();
}

void MainWindow::pauseSending()
{
    if (state_ != Sending)
        return;
    sender_.setPaused(true);
    setState(Paused);
    ui->stateLabel->setText(tr("正在暂停…"));
    setMessage(tr("已请求暂停。点击“继续输入”后会重新倒计时，方便切回目标输入框。"));
}

void MainWindow::stopSending()
{
    if (state_ == Idle || state_ == Stopping)
        return;
    countdownTimer_.stop();
    if (state_ == Countdown && !resuming_) {
        setState(Idle);
        ui->stateLabel->setText(tr("已取消"));
        setMessage(tr("倒计时已取消，尚未发送任何字符。"));
        return;
    }
    setState(Stopping);
    ui->stateLabel->setText(tr("正在停止…"));
    sender_.requestStop();
}

void MainWindow::updateProgress(qsizetype sent, qsizetype total, qint64 remainingMs)
{
    ui->progressLabel->setText(tr("%1 / %2 字符").arg(sent).arg(total));
    ui->progressBar->setRange(0, 1000);
    ui->progressBar->setValue(total ? static_cast<int>(static_cast<double>(sent) / total * 1000) : 0);
    ui->progressBar->setAccessibleDescription(ui->progressLabel->text());
    ui->remainingLabel->setText(total ? tr("预计剩余 %1").arg(duration(remainingMs)) : tr("预计剩余 —"));
}

void MainWindow::sendingFinished()
{
    // Join before enabling Start so a new job cannot race the tail of QThread::run().
    sender_.wait();
    countdownTimer_.stop();
    setState(Idle);
    const auto result = sender_.result();
    updateProgress(sender_.sentCount(), jobPlan_.units.size(),
                   jobPlan_.remainingMs(sender_.sentCount(), jobOptions_.speed));
    if (result == KeySenderThread::Failed) {
        ui->stateLabel->setText(tr("输入未完成"));
        setMessage(sender_.error(), true);
    } else if (result == KeySenderThread::Stopped) {
        ui->stateLabel->setText(tr("已停止"));
        setMessage(tr("已停止，已发送 %1 / %2 个字符。").arg(sender_.sentCount()).arg(jobPlan_.units.size()));
    } else {
        ui->stateLabel->setText(tr("发送完成"));
        setMessage(tr("%1 个字符已发送。请在目标应用中确认输入结果。").arg(sender_.sentCount()));
        if (ui->notifyCheck->isChecked())
            QApplication::alert(this, 3000);
    }
    refreshPermission();
}
