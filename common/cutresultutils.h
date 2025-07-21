#pragma once
#include "model/cutresult.h"
#include "model/reusablestockentry.h"

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
static inline QVector<ReusableStockEntry> toReusableEntries(const QVector<CutResult>& input) {
    QVector<ReusableStockEntry> result;

    for (const CutResult& res : input) {
        ReusableStockEntry entry;
        entry.materialId = res.materialId;
        entry.availableLength_mm = res.waste;
        entry.source = convertToLeftoverSource(res.source);

        result.append(entry);
    }

    return result;
}

// 🔁 Egyedi CutResult → reusable darab
static inline ReusableStockEntry toReusableEntry(const CutResult& res) {
    ReusableStockEntry entry;
    entry.materialId = res.materialId;
    entry.availableLength_mm = res.waste;
    entry.source = convertToLeftoverSource(res.source);;

    return entry;
}

}

