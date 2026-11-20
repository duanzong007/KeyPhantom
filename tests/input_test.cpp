#include "inputplan.h"
#include "keysenderthread.h"

#include <QElapsedTimer>
#include <QSemaphore>
#include <QSignalSpy>
#include <QtTest>
#include <atomic>

class InputTest : public QObject
{
    Q_OBJECT
private slots:
    void preservesGraphemes()
    {
        InputOptions options;
        const auto plan = InputPlan::fromText(QString::fromUtf8("A😀é👩‍💻𠮷"), options);
        QCOMPARE(plan.units, QStringList({QStringLiteral("A"), QString::fromUtf8("😀"),
            QString::fromUtf8("é"), QString::fromUtf8("👩‍💻"), QString::fromUtf8("𠮷")}));
    }

    void normalizesLineEndings()
    {
        const auto plan = InputPlan::fromText(QStringLiteral("a\r\nb\rc\n\n"), {});
        QCOMPARE(plan.units.join(QString()), QStringLiteral("a\nb\nc\n\n"));
        QCOMPARE(plan.units.count(QStringLiteral("\n")), 4);
        QVERIFY(InputPlan::fromText({}, {}).units.isEmpty());
    }

    void expandsTabs()
    {
        InputOptions options;
        QCOMPARE(InputPlan::fromText(QStringLiteral("a\tb"), options).units.size(), 3);
        options.tabMode = InputOptions::Spaces;
        options.tabWidth = 4;
        QCOMPARE(InputPlan::fromText(QStringLiteral("a\tb"), options).units.join(QString()), QStringLiteral("a    b"));
        options.tabWidth = 99;
        QCOMPARE(InputPlan::fromText(QStringLiteral("\t"), options).units.size(), 8);
    }

    void estimatesBatchBoundaries()
    {
        const auto plan = InputPlan::fromText(QStringLiteral("123456"), {});
        QCOMPARE(plan.remainingMs(0, InputOptions::Normal), 100);
        QCOMPARE(plan.remainingMs(1, InputOptions::Normal), 100);
        QCOMPARE(plan.remainingMs(5, InputOptions::Normal), 60);
        QCOMPARE(plan.remainingMs(6, InputOptions::Normal), 0);
        QCOMPARE(InputPlan::fromText(QStringLiteral("x"), {}).remainingMs(0, InputOptions::Slow), 0);
    }

    void sendsWholeCharactersAndOptions()
    {
        QStringList received;
        bool shift = true;
        KeySenderThread sender(nullptr, [&](const QString &text, bool shiftEnter) {
            received.append(text);
            shift = shift && shiftEnter;
            return QString();
        }, [] { return true; });
        InputOptions options;
        options.speed = InputOptions::Ultra;
        options.newlineMode = InputOptions::ShiftEnter;
        const auto plan = InputPlan::fromText(QString::fromUtf8("😀\r\n\t𠮷"), options);
        QVERIFY(sender.configure(plan, options));
        QSignalSpy progress(&sender, &KeySenderThread::progress);
        sender.start();
        QVERIFY(sender.wait(2000));
        QCOMPARE(sender.result(), KeySenderThread::Completed);
        QCOMPARE(received, plan.units);
        QVERIFY(shift);
        QCOMPARE(sender.sentCount(), plan.units.size());
        QVERIFY(!progress.isEmpty());
        QCOMPARE(progress.last().at(0).toLongLong(), qint64(plan.units.size()));
        QCOMPARE(progress.last().at(2).toLongLong(), 0);
    }

    void reportsFailureWithoutCountingUnsentCharacters()
    {
        int calls = 0;
        KeySenderThread sender(nullptr, [&](const QString &, bool) {
            return ++calls == 2 ? QStringLiteral("injected failure") : QString();
        }, [] { return true; });
        InputOptions options;
        options.speed = InputOptions::Ultra;
        QVERIFY(sender.configure(InputPlan::fromText(QStringLiteral("abc"), options), options));
        sender.start();
        QVERIFY(sender.wait(2000));
        QCOMPARE(sender.result(), KeySenderThread::Failed);
        QCOMPARE(sender.sentCount(), 1);
        QCOMPARE(sender.error(), QStringLiteral("injected failure"));
    }

    void blocksMissingPermission()
    {
        int calls = 0;
        KeySenderThread sender(nullptr, [&](const QString &, bool) { ++calls; return QString(); },
                               [] { return false; });
        QVERIFY(sender.configure(InputPlan::fromText(QStringLiteral("abc"), {}), {}));
        sender.start();
        QVERIFY(sender.wait(1000));
        QCOMPARE(calls, 0);
        QCOMPARE(sender.result(), KeySenderThread::Failed);
        QCOMPARE(sender.sentCount(), 0);
        QVERIFY(!sender.error().isEmpty());
    }

    void detectsRevokedPermission()
    {
        bool allowed = true;
        KeySenderThread sender(nullptr, [&](const QString &, bool) { allowed = false; return QString(); },
                               [&] { return allowed; });
        InputOptions options;
        options.speed = InputOptions::Slow;
        QVERIFY(sender.configure(InputPlan::fromText(QStringLiteral("abc"), options), options));
        sender.start();
        QVERIFY(sender.wait(2000));
        QCOMPARE(sender.result(), KeySenderThread::Failed);
        QCOMPARE(sender.sentCount(), 1);
    }

    void pauseResumeAndStopAreInterruptible()
    {
        QSemaphore first;
        std::atomic_int calls{0};
        KeySenderThread sender(nullptr, [&](const QString &, bool) {
            ++calls;
            first.release();
            return QString();
        }, [] { return true; });
        InputOptions options;
        options.speed = InputOptions::Slow;
        const auto plan = InputPlan::fromText(QStringLiteral("abcdef"), options);
        QVERIFY(sender.configure(plan, options));
        QSignalSpy paused(&sender, &KeySenderThread::pauseAcknowledged);
        QSignalSpy progress(&sender, &KeySenderThread::progress);
        sender.start();
        QVERIFY(first.tryAcquire(1, 1000));
        QVERIFY(!sender.configure(plan, options));
        sender.setPaused(true);
        QTRY_COMPARE_WITH_TIMEOUT(paused.count(), 1, 1000);
        const int count = calls.load();
        QCOMPARE(progress.last().at(0).toInt(), count);
        QTest::qWait(100);
        QCOMPARE(calls.load(), count);
        sender.setPaused(false);
        QVERIFY(first.tryAcquire(1, 1500));
        sender.setPaused(true);
        QTRY_COMPARE_WITH_TIMEOUT(paused.count(), 2, 1000);
        QElapsedTimer stopTime;
        stopTime.start();
        sender.requestStop();
        QVERIFY(sender.wait(250));
        QVERIFY(stopTime.elapsed() < 250);
        QCOMPARE(sender.result(), KeySenderThread::Stopped);
        QVERIFY(sender.sentCount() < plan.units.size());

        // Reuse after a joined thread: previous stop and pause flags must be reset.
        options.speed = InputOptions::Ultra;
        QVERIFY(sender.configure(InputPlan::fromText(QStringLiteral("z"), options), options));
        sender.start();
        QVERIFY(sender.wait(1000));
        QCOMPARE(sender.result(), KeySenderThread::Completed);
        QCOMPARE(sender.sentCount(), 1);
    }

    void stopInterruptsBatchWait()
    {
        QSemaphore first;
        KeySenderThread sender(nullptr, [&](const QString &, bool) { first.release(); return QString(); },
                               [] { return true; });
        InputOptions options;
        options.speed = InputOptions::Slow;
        QVERIFY(sender.configure(InputPlan::fromText(QStringLiteral("ab"), options), options));
        sender.start();
        QVERIFY(first.tryAcquire(1, 1000));
        sender.requestStop();
        QVERIFY(sender.wait(250));
        QCOMPARE(sender.result(), KeySenderThread::Stopped);
        QCOMPARE(sender.sentCount(), 1);
    }
};

QTEST_GUILESS_MAIN(InputTest)
#include "input_test.moc"
