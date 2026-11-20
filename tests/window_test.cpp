#include "appearance.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class MainWindowTest : public QObject
{
    Q_OBJECT
    QTemporaryDir settingsDirectory;
private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("KeyPhantomTests"));
        QCoreApplication::setApplicationName(QStringLiteral("WindowTest"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
        applyApplicationAppearance();
    }

    void init() { QSettings().clear(); }

    void previewsAndCancelsCountdown()
    {
        MainWindow window;
        window.ui->textEdit->setPlainText(QString::fromUtf8("你好 😀\nsecond line"));
        window.refreshPlan();
        QVERIFY(window.ui->countLabel->text().contains(QStringLiteral("2 行")));
        window.ui->delaySpin->setValue(3);
        window.beginCountdown(false); // Exercise countdown without starting any native input.
        QCOMPARE(window.state_, MainWindow::Countdown);
        QVERIFY(window.ui->stateLabel->text().contains(QStringLiteral("3")));
        QVERIFY(window.ui->textEdit->isReadOnly());
        QVERIFY(!window.ui->settingsCard->isEnabled());
        QVERIFY(!window.ui->stopButton->isVisible());
        QTest::mouseClick(window.ui->startButton, Qt::LeftButton);
        QCOMPARE(window.state_, MainWindow::Idle);
        QVERIFY(!window.countdownTimer_.isActive());
        QVERIFY(!window.sender_.isRunning());
        QVERIFY(!window.ui->textEdit->isReadOnly());
    }

    void remembersPreferencesWithoutText()
    {
        {
            MainWindow window;
            window.ui->speedCombo->setCurrentIndex(InputOptions::Fast);
            window.ui->delaySpin->setValue(7);
            window.ui->newlineCombo->setCurrentIndex(InputOptions::ShiftEnter);
            window.ui->tabCombo->setCurrentIndex(InputOptions::Spaces);
            window.ui->tabWidthSpin->setValue(2);
            window.ui->notifyCheck->setChecked(true);
            window.ui->textEdit->setPlainText(QStringLiteral("not saved"));
        }
        MainWindow restored;
        QCOMPARE(restored.ui->speedCombo->currentIndex(), int(InputOptions::Fast));
        QCOMPARE(restored.ui->delaySpin->value(), 7);
        QCOMPARE(restored.ui->newlineCombo->currentIndex(), int(InputOptions::ShiftEnter));
        QCOMPARE(restored.ui->tabCombo->currentIndex(), int(InputOptions::Spaces));
        QCOMPARE(restored.ui->tabWidthSpin->value(), 2);
        QVERIFY(restored.ui->notifyCheck->isChecked());
        QVERIFY(restored.ui->textEdit->toPlainText().isEmpty());
        for (const auto &key : QSettings().allKeys())
            QVERIFY(!QSettings().value(key).toString().contains(QStringLiteral("not saved")));
    }

    void closesDuringCountdown()
    {
        MainWindow window;
        window.beginCountdown(false);
        window.close();
        QVERIFY(!window.countdownTimer_.isActive());
        QVERIFY(!window.sender_.isRunning());
    }

    void homeAndSettingsAreSeparate()
    {
        MainWindow window;
        window.show();
        QCOMPARE(window.ui->pageStack->currentWidget(), window.ui->homePage);
        QVERIFY(!window.ui->settingsCard->isVisible());
        QVERIFY(!window.ui->notifyCheck->isVisible());
        QVERIFY(!window.ui->progressGroup->isVisible());
        QVERIFY(!window.ui->stopButton->isVisible());
        QVERIFY(window.ui->textEdit->isVisible());
        QVERIFY(window.ui->startButton->isVisible());
        QTest::mouseClick(window.ui->settingsButton, Qt::LeftButton);
        QCOMPARE(window.ui->pageStack->currentWidget(), window.ui->settingsPage);
        QVERIFY(window.ui->speedCombo->isVisible());
        QVERIFY(window.ui->notifyCheck->isVisible());
        QVERIFY(!window.ui->textEdit->isVisible());
        QTest::mouseClick(window.ui->settingsButton, Qt::LeftButton);
        QCOMPARE(window.ui->pageStack->currentWidget(), window.ui->homePage);
    }

    void compactLayoutDoesNotOverlap()
    {
        MainWindow window;
        window.resize(window.minimumSize());
        window.show();
        QTest::qWait(50);
        const int editorBottom = window.ui->textEdit->mapTo(window.ui->homePage,
            QPoint(0, window.ui->textEdit->height())).y();
        const int metaTop = window.ui->countLabel->mapTo(window.ui->homePage, QPoint()).y();
        const int buttonTop = window.ui->startButton->mapTo(window.ui->homePage, QPoint()).y();
        QVERIFY(editorBottom <= metaTop);
        QVERIFY(window.ui->textEdit->height() >= window.ui->textEdit->minimumHeight());
        QVERIFY(metaTop < buttonTop);
        QVERIFY(window.ui->startButton->geometry().right() <= window.ui->homePage->width());
    }

    void refusesToStartInOwnWindow()
    {
        MainWindow window;
        window.ui->textEdit->setPlainText(QStringLiteral("must not be sent"));
        window.show();
        window.activateWindow();
        QTRY_VERIFY_WITH_TIMEOUT(window.isActiveWindow(), 1000);
        window.beginCountdown(false);
        window.countdownMs_ = 0;
        window.tickCountdown();
        QCOMPARE(window.state_, MainWindow::Idle);
        QVERIFY(!window.sender_.isRunning());
        QVERIFY(!window.countdownTimer_.isActive());
    }

    void returningToWindowDoesNotResumeFromActivationClick()
    {
        MainWindow window;
        window.setState(MainWindow::Sending);
        QMetaObject::invokeMethod(qApp, "applicationStateChanged", Qt::DirectConnection,
                                  Q_ARG(Qt::ApplicationState, Qt::ApplicationActive));
        QCOMPARE(window.state_, MainWindow::Paused);
        QVERIFY(window.activationPause_);
        QVERIFY(!window.ui->startButton->isEnabled());
        window.startOrResume();
        QCOMPARE(window.state_, MainWindow::Paused);
        QVERIFY(!window.countdownTimer_.isActive());
        QTRY_VERIFY_WITH_TIMEOUT(!window.activationPause_, 1000);
    }

    void cancellingResumeCountdownKeepsJobPaused()
    {
        MainWindow window;
        window.setState(MainWindow::Paused);
        window.beginCountdown(true);
        QCOMPARE(window.state_, MainWindow::Countdown);
        QTest::mouseClick(window.ui->startButton, Qt::LeftButton);
        QCOMPARE(window.state_, MainWindow::Paused);
        QVERIFY(!window.countdownTimer_.isActive());
        QVERIFY(!window.sender_.isRunning());
    }

    void renderPreview()
    {
        const QString path = qEnvironmentVariable("KEYPHANTOM_PREVIEW_PATH");
        if (path.isEmpty())
            return;
        MainWindow window;
        window.resize(760, 640);
        window.ui->textEdit->setPlainText(QString::fromUtf8(
            "把想说的话，慢慢输入。\n\n"
            "支持中文、英文，以及完整的 Emoji 👩‍💻\n"
            "换行与 Tab 都可以按你的习惯设置。\n\n"
            "准备好后点击「开始输入」，再切换到目标窗口。"));
        window.ui->tabCombo->setCurrentIndex(InputOptions::Spaces);
        window.refreshPlan();
        window.show();
        QTest::qWait(100);
        QVERIFY(window.grab().save(path));
        window.resize(window.minimumSize());
        QTest::qWait(50);
        QVERIFY(window.grab().save(path + QStringLiteral(".compact.png")));
        window.resize(760, 640);
        QTest::mouseClick(window.ui->settingsButton, Qt::LeftButton);
        QTest::qWait(50);
        QVERIFY(window.grab().save(path + QStringLiteral(".settings.png")));
        QTest::mouseClick(window.ui->settingsButton, Qt::LeftButton);
        window.beginCountdown(false);
        QTest::qWait(50);
        QVERIFY(window.grab().save(path + QStringLiteral(".countdown.png")));
        window.stopSending();
    }
};

QTEST_MAIN(MainWindowTest)
#include "window_test.moc"
