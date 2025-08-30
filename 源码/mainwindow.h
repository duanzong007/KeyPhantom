#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <atomic>

#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class KeySenderThread : public QThread
{
    Q_OBJECT
public:
    enum Speed { Slow = 0, Normal, Fast, Ultra };

    explicit KeySenderThread(QObject *p = nullptr) : QThread(p) {}

    void configure(const QString &txt, Speed spd)
    { text_ = txt; speed_ = spd; stop_.store(false); }

    void requestStop() { stop_.store(true); }

signals:
    void finished(bool byUserStop);

protected:
    void run() override;

private:
    void sendUnicode(QChar ch);

#ifdef Q_OS_MAC
    void sendEnter();
    void sendTab();
    void sendKeycode(uint16_t kc);
#endif

    QString          text_;
    Speed            speed_ = Normal;
    std::atomic_bool stop_{false};

#ifdef Q_OS_WIN
    void sendVk(unsigned short vk);
#endif
};

/* ============================= 主窗口 ============================== */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onMainBtnClicked();   // 单一按钮
    void beginAfterDelay();    // 3 秒后启动
    void onSendFinished(bool byUserStop);

private:
    enum State { Idle, Waiting, Sending };
    State                 state_ = Idle;

    KeySenderThread::Speed currentSpeed() const;

    Ui::MainWindow *ui;
    KeySenderThread sender_;
    QTimer          delayTimer_;

#ifdef Q_OS_MAC
    bool checkMacPermissions();
#endif

};

#endif // MAINWINDOW_H
