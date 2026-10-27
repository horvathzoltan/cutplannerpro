#pragma once

#include <QUuid>

enum class SearchTarget
{
    Summary,
    Order
};

struct SearchHit
{
    SearchTarget target;

    QUuid id;

    int column;
};