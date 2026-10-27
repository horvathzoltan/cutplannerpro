#pragma once

#include "searchhit.h"
#include <QVector>

struct SearchResult
{
    QVector<SearchHit> hits;

    bool hasHits() const
    {
        return !hits.isEmpty();
    }
};