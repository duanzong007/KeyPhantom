#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <QShortcut>
#endif

/* =================================================================== */
/*                         KeySenderThread                              */
/* =================================================================== */
void KeySenderThread::run()
{
    int batch{}, charInt{}, batchInt{};
    switch (speed_) {
    case Slow :  batch=1;  charInt=20; batchInt=300; break;
    case Normal: batch=5;  charInt=10; batchInt= 50; break;
    case Fast :  batch=20; charInt= 2; batchInt= 20; break;
    case Ultra:  batch=50; charInt= 0; batchInt= 10; break;
    }

    int cnt = 0;
    for (QChar ch : text_)
    {

#ifdef Q_OS_WIN
        if (stop_.load()) { emit finished(true); return; }

        if (ch == u'\n' || ch == u'\r') {
            sendVk(VK_RETURN);
        }
        else if (ch == u'\t') {
            sendVk(VK_TAB);
        }
        else {
            sendUnicode(ch);
        }
        msleep(charInt);
        if (++cnt >= batch) { cnt = 0; msleep(batchInt); }
#endif

#ifdef Q_OS_MAC
        if (stop_.load()) { emit finished(true); return; }

        if (ch == u'\n' || ch == u'\r') {
            sendEnter();
        } else if (ch == u'\t') {
            sendTab();
        } else {
            sendUnicode(ch);
        }

        QThread::msleep(charInt);
        if (++cnt >= batch) { cnt = 0; QThread::msleep(batchInt); }
#endif

    }

    emit finished(false);
}

#ifdef Q_OS_WIN

void KeySenderThread::sendUnicode(QChar ch)
{
    INPUT ip[2]{};
    ip[0].type = INPUT_KEYBOARD;
    ip[0].ki.wScan = ch.unicode();
    ip[0].ki.dwFlags = KEYEVENTF_UNICODE;        // down
    ip[1] = ip[0];
    ip[1].ki.dwFlags |= KEYEVENTF_KEYUP;         // up
    SendInput(2, ip, sizeof(INPUT));
}

void KeySenderThread::sendVk(unsigned short vk)
{
    INPUT ip[2]{};
    ip[0].type       = INPUT_KEYBOARD;
    ip[0].ki.wVk     = vk;               // 虚拟键
    ip[0].ki.dwFlags = 0;                // down

    ip[1]            = ip[0];
    ip[1].ki.dwFlags = KEYEVENTF_KEYUP;  // up

    SendInput(2, ip, sizeof(INPUT));
}

#endif

#ifdef Q_OS_MAC

static inline void postEvent(CGEventRef e)
{
    CGEventPost(kCGSessionEventTap, e);
}

void KeySenderThread::sendUnicode(QChar ch)
{
    UniChar c = ch.unicode();

    CGEventRef keyDown = CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)0, true);
    CGEventKeyboardSetUnicodeString(keyDown, 1, &c);

    CGEventRef keyUp = CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)0, false);
    CGEventKeyboardSetUnicodeString(keyUp, 1, &c);

    postEvent(keyDown);
    postEvent(keyUp);

    CFRelease(keyDown);
    CFRelease(keyUp);
}

void KeySenderThread::sendKeycode(uint16_t kc)
{
    CGEventRef keyDown = CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)kc, true);
    CGEventRef keyUp   = CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)kc, false);

    postEvent(keyDown);
    postEvent(keyUp);

    CFRelease(keyDown);
    CFRelease(keyUp);
}

void KeySenderThread::sendEnter() { sendKeycode(kVK_Return); }
void KeySenderThread::sendTab()   { sendKeycode(kVK_Tab);    }

#endif



/* =================================================================== */
/*                            MainWindow                                */
/* =================================================================== */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    checkMacPermissions();

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    connect(shortcut, &QShortcut::activated, this, [this]() {
        this->hide();
    });

    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && this->isHidden()) {
            this->showNormal();
            this->raise();
            this->activateWindow();
        }
    });
    
#endif


    ui->comboBox->addItems({u8"慢速", u8"正常", u8"快速", u8"超高速"});
    ui->comboBox->setCurrentIndex(1);

    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::onMainBtnClicked);

    delayTimer_.setSingleShot(true);
    connect(&delayTimer_, &QTimer::timeout,
            this, &MainWindow::beginAfterDelay);

    connect(&sender_, &KeySenderThread::finished,
            this, &MainWindow::onSendFinished);
}

MainWindow::~MainWindow()
{
    sender_.requestStop();
    sender_.quit();
    sender_.wait();
    delete ui;
}

void MainWindow::onMainBtnClicked()
{
    if (state_ == Idle) {
        if (ui->plainTextEdit->toPlainText().isEmpty()) {
            QMessageBox::information(this, u8"提示", u8"文本为空，无法发送！");
            return;
        }
        state_ = Waiting;
        ui->pushButton->setEnabled(false);
        ui->pushButton->setText(u8"3 秒后开始…");
        delayTimer_.start(3000);
    }
    else if (state_ == Sending) {
        ui->pushButton->setEnabled(false);
        sender_.requestStop();
    }
}

void MainWindow::beginAfterDelay()
{
    sender_.configure(ui->plainTextEdit->toPlainText(), currentSpeed());
    sender_.start();

    state_ = Sending;
    ui->pushButton->setText(u8"停止");
    ui->pushButton->setEnabled(true);
}

void MainWindow::onSendFinished(bool byUserStop)
{
    state_ = Idle;
    ui->pushButton->setText(u8"粘贴");
    ui->pushButton->setEnabled(true);

    QMessageBox::information(this, u8"完成",
                             byUserStop ? u8"已手动停止输入！" : u8"全部字符已发送完毕！");
}

KeySenderThread::Speed MainWindow::currentSpeed() const
{
    return static_cast<KeySenderThread::Speed>(ui->comboBox->currentIndex());
}

#ifdef Q_OS_MAC

#include <QMessageBox>
#include <ApplicationServices/ApplicationServices.h>

bool MainWindow::checkMacPermissions()
{
    // 检查辅助功能权限
    bool accessibilityGranted = AXIsProcessTrusted();
    if (!accessibilityGranted) {
        QMessageBox::warning(this, "权限提示",
                             "未开启辅助功能权限。\n"
                             "请打开：系统设置 → 隐私与安全性 → 辅助功能 → 允许此应用控制电脑。");
        return false;
    }

    return true;
}

#endif


