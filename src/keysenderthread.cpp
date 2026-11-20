#include "keysenderthread.h"
#include "platforminput.h"

#include <QElapsedTimer>
#include <QMutexLocker>
#include <utility>

KeySenderThread::KeySenderThread(QObject *parent, SendFunction send, PermissionCheck permissionCheck)
    : QThread(parent), send_(send ? std::move(send) : PlatformInput::send),
      permissionCheck_(permissionCheck ? std::move(permissionCheck) : PlatformInput::hasPermission)
{
}

KeySenderThread::~KeySenderThread()
{
    requestStop();
    wait();
}

bool KeySenderThread::configure(InputPlan plan, InputOptions options)
{
    if (isRunning())
        return false;
    QMutexLocker lock(&mutex_);
    plan_ = std::move(plan);
    options_ = options;
    stopped_ = paused_ = pauseAnnounced_ = false;
    sent_ = 0;
    result_ = Completed;
    error_.clear();
    return true;
}

void KeySenderThread::requestStop()
{
    QMutexLocker lock(&mutex_);
    stopped_ = true;
    condition_.wakeAll();
}

void KeySenderThread::setPaused(bool paused)
{
    QMutexLocker lock(&mutex_);
    paused_ = paused;
    condition_.wakeAll();
}

bool KeySenderThread::waitReady(int milliseconds)
{
    QMutexLocker lock(&mutex_);
    qint64 remaining = milliseconds;
    while (!stopped_) {
        if (paused_) {
            if (!pauseAnnounced_) {
                pauseAnnounced_ = true;
                emit progress(sent_, plan_.units.size(), plan_.remainingMs(sent_, options_.speed));
                emit pauseAcknowledged();
            }
            condition_.wait(&mutex_);
        } else {
            pauseAnnounced_ = false;
            if (remaining <= 0)
                return true;
            QElapsedTimer elapsed;
            elapsed.start();
            condition_.wait(&mutex_, static_cast<unsigned long>(remaining));
            remaining -= elapsed.elapsed();
        }
    }
    return false;
}

void KeySenderThread::run()
{
    const auto timing = InputPlan::profile(options_.speed);
    QElapsedTimer reportTimer;
    QElapsedTimer permissionTimer;
    reportTimer.start();
    permissionTimer.start();
    emit progress(0, plan_.units.size(), plan_.remainingMs(0, options_.speed));

    for (const QString &unit : plan_.units) {
        if (!waitReady()) {
            result_ = Stopped;
            break;
        }
        if (sent_ == 0 || permissionTimer.elapsed() >= 250) {
            permissionTimer.restart();
            if (!permissionCheck_()) {
                error_ = QStringLiteral("辅助功能权限未开启或已被撤销，请授权后重新开始。");
                result_ = Failed;
                break;
            }
        }
        error_ = send_(unit, options_.newlineMode == InputOptions::ShiftEnter);
        if (!error_.isEmpty()) {
            result_ = Failed;
            break;
        }
        ++sent_;
        if (reportTimer.elapsed() >= 50 || sent_ == 1 || sent_ == plan_.units.size()) {
            emit progress(sent_, plan_.units.size(), plan_.remainingMs(sent_, options_.speed));
            reportTimer.restart();
        }
        if (sent_ < plan_.units.size()) {
            const int delay = timing.characterMs + (sent_ % timing.batchSize == 0 ? timing.batchMs : 0);
            if (!waitReady(delay)) {
                result_ = Stopped;
                break;
            }
        }
    }
    emit progress(sent_, plan_.units.size(), plan_.remainingMs(sent_, options_.speed));
}
