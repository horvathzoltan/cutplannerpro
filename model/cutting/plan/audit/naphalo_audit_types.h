#pragma once

#include <QString>
#include <QStringList>

struct CountPerType {
    int cipzaros = 0;
    int sines = 0;
    int bowdenes = 0;

    int total = 0;
    int good = 0;
    int bad = 0;

    QStringList badRefs;
};
