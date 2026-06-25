#pragma once
#include <QString>

static inline bool matchPrefix(const QString& barcode, const QString& prefix)
{
    // Pontos egyezés: NP-T
    if (barcode == prefix)
        return true;

    // Prefix + '-' egyezés: NP-T-9010
    if (barcode.startsWith(prefix + "-"))
        return true;

    return false;
}
