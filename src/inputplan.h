#pragma once

#include <QString>
#include <QStringList>

struct InputOptions
{
    enum Speed { Slow, Normal, Fast, Ultra };
    enum NewlineMode { Enter, ShiftEnter };
    enum TabMode { TabKey, Spaces };

    Speed speed = Normal;
    NewlineMode newlineMode = Enter;
    TabMode tabMode = TabKey;
    int tabWidth = 4;
};

struct SpeedProfile
{
    int batchSize;
    int characterMs;
    int batchMs;
};

struct InputPlan
{
    QStringList units;

    static InputPlan fromText(QString text, const InputOptions &options);
    static SpeedProfile profile(InputOptions::Speed speed);
    qint64 remainingMs(qsizetype sent, InputOptions::Speed speed) const;
};
