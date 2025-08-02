#pragma once
#include "model/cutresult.h"
#include "model/leftoverstockentry.h"

namespace CutResultUtils
{

static inline LeftoverSource convertToLeftoverSource(CutResultSource source) {
    switch (source) {
    case CutResultSource::FromStock: return LeftoverSource::Optimization;
    case CutResultSource::FromReusable: return LeftoverSource::Manual;
    case CutResultSource::Unknown:
    default: return LeftoverSource::Undefined;
    }
}


// 🔁 Több CutResult → reusable készlet
static inline QVector<LeftoverStockEntry> toReusableEntries(const QVector<CutResult>& input) {
    QVector<LeftoverStockEntry> result;

    for (const CutResult& res : input) {
        LeftoverStockEntry entry;
        entry.materialId = res.materialId;
        entry.availableLength_mm = res.waste;
        entry.source = convertToLeftoverSource(res.source);

        result.append(entry);
    }

    return result;
}

// 🔁 Egyedi CutResult → reusable darab
static inline LeftoverStockEntry toReusableEntry(const CutResult& res) {
    LeftoverStockEntry entry;
    entry.materialId = res.materialId;
    entry.availableLength_mm = res.waste;
    entry.source = convertToLeftoverSource(res.source);;

    return entry;
}

}

