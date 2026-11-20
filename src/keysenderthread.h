#pragma once

#include "inputplan.h"

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <functional>

class KeySenderThread : public QThread
{
    Q_OBJECT
public:
    using SendFunction = std::function<QString(const QString &, bool)>;
    using PermissionCheck = std::function<bool()>;
    enum Result { Completed, Stopped, Failed };
    Q_ENUM(Result)

    explicit KeySenderThread(QObject *parent = nullptr, SendFunction send = {},
                             PermissionCheck permissionCheck = {});
    ~KeySenderThread() override;
    bool configure(InputPlan plan, InputOptions options);
    void requestStop();
    void setPaused(bool paused);
    Result result() const { return result_; } // Read only after QThread::finished / wait().
    QString error() const { return error_; }
    qsizetype sentCount() const { return sent_; }

signals:
    void progress(qsizetype sent, qsizetype total, qint64 remainingMs);
    void pauseAcknowledged();

protected:
    void run() override;

private:
    bool waitReady(int milliseconds = 0);
    InputPlan plan_;
    InputOptions options_;
    SendFunction send_;
    PermissionCheck permissionCheck_;
    QMutex mutex_;
    QWaitCondition condition_;
    bool stopped_ = false;
    bool paused_ = false;
    bool pauseAnnounced_ = false;
    Result result_ = Completed;
    QString error_;
    qsizetype sent_ = 0;
};
