#pragma once

#include "globalstophotkey.h"
#include "keysenderthread.h"

#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    friend class MainWindowTest;
    enum State { Idle, Countdown, Sending, Paused, Stopping };
    InputOptions currentOptions() const;
    void restoreSettings();
    void saveSettings() const;
    void refreshPlan();
    void refreshPermission();
    void updateControls();
    void setState(State state);
    void setMessage(const QString &message, bool error = false);
    void startOrResume();
    void beginCountdown(bool resume);
    void tickCountdown();
    void pauseSending();
    void stopSending();
    void sendingFinished();
    void updateProgress(qsizetype sent, qsizetype total, qint64 remainingMs);
    static QString duration(qint64 milliseconds);

    Ui::MainWindow *ui;
    KeySenderThread sender_;
    GlobalStopHotkey stopHotkey_;
    QTimer countdownTimer_;
    QTimer permissionTimer_;
    QTimer previewTimer_;
    QElapsedTimer countdownClock_;
    InputPlan previewPlan_;
    InputPlan jobPlan_;
    InputOptions jobOptions_;
    State state_ = Idle;
    int countdownMs_ = 3000;
    bool resuming_ = false;
    bool permissionGranted_ = false;
    bool activationPause_ = false;
};
