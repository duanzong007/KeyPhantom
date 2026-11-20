#include "inputplan.h"

#include <QTextBoundaryFinder>

InputPlan InputPlan::fromText(QString text, const InputOptions &options)
{
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(u'\r', u'\n');
    if (options.tabMode == InputOptions::Spaces)
        text.replace(QStringLiteral("\t"), QString(qBound(1, options.tabWidth, 8), u' '));

    InputPlan plan;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    qsizetype start = 0;
    for (qsizetype end = finder.toNextBoundary(); end != -1; end = finder.toNextBoundary()) {
        plan.units.append(text.mid(start, end - start));
        start = end;
    }
    return plan;
}

SpeedProfile InputPlan::profile(InputOptions::Speed speed)
{
    switch (speed) {
    case InputOptions::Slow: return {1, 20, 300};
    case InputOptions::Fast: return {20, 2, 20};
    case InputOptions::Ultra: return {50, 0, 10};
    default: return {5, 10, 50};
    }
}

qint64 InputPlan::remainingMs(qsizetype sent, InputOptions::Speed speed) const
{
    if (units.isEmpty() || sent >= units.size())
        return 0;
    const auto timing = profile(speed);
    const qint64 remainingGaps = units.size() - qMax<qsizetype>(1, sent);
    const qint64 batches = (units.size() - 1) / timing.batchSize
                          - (qMax<qsizetype>(1, sent) - 1) / timing.batchSize;
    return remainingGaps * timing.characterMs + batches * timing.batchMs;
}
