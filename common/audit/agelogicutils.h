#pragma once

#include <QDate>
#include <QDateTime>

#include "common/audit/agestate.h"

namespace AgeLogicUtils
{

inline AgeState determine(const QDateTime& lastSeenAt,
                          int notFoundCount = 0)
{
    if (notFoundCount > 0)
        return AgeState::Missing;

    if (!lastSeenAt.isValid())
        return AgeState::Old;

    const QDateTime now = QDateTime::currentDateTime();

    if (lastSeenAt >= now.addSecs(-3600))
        return AgeState::ExtraFresh;

    if (lastSeenAt.date() == now.date())
        return AgeState::Fresh;

    if (lastSeenAt.date() == now.date().addDays(-1))
        return AgeState::Stale;

    return AgeState::Old;
}

inline bool isMissing(const QDateTime& lastSeenAt,
                      int notFoundCount = 0)
{
    return determine(lastSeenAt, notFoundCount)
    == AgeState::Missing;
}

inline bool isExtraFresh(const QDateTime& lastSeenAt,
                         int notFoundCount = 0)
{
    return determine(lastSeenAt, notFoundCount)
    == AgeState::ExtraFresh;
}

inline bool isFresh(const QDateTime& lastSeenAt,
                    int notFoundCount = 0)
{
    auto a =  determine(lastSeenAt, notFoundCount);
    bool isFresh =  a == AgeState::Fresh || a== AgeState::ExtraFresh;
    return isFresh;
}

inline bool isStale(const QDateTime& lastSeenAt,
                    int notFoundCount = 0)
{
    return determine(lastSeenAt, notFoundCount)
    == AgeState::Stale;
}

inline bool isOld(const QDateTime& lastSeenAt,
                  int notFoundCount = 0)
{
    return determine(lastSeenAt, notFoundCount)
    == AgeState::Old;
}

}
