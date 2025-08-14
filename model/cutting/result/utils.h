#pragma once
#include "model/cutting/result/resultmodel.h"
#include "model/cutting/result/source.h"
#include "model/cutting/result/leftoversource.h"
#include "model/leftoverstockentry.h"

namespace Cutting{
namespace Result{

namespace Utils
{

static inline LeftoverSource convertToLeftoverSource(Source source) {
    switch (source) {
    case Source::FromStock: return LeftoverSource::Optimization;
    case Source::FromReusable: return LeftoverSource::Manual;
    case Source::Unknown:
    default: return LeftoverSource::Undefined;
    }
}


// 🔁 Több CutResult → reusable készlet
static inline QVector<LeftoverStockEntry> toReusableEntries(const QVector<ResultModel>& input) {
    QVector<LeftoverStockEntry> result;

    for (const ResultModel& res : input) {
        LeftoverStockEntry entry;
        entry.materialId = res.materialId;
        entry.availableLength_mm = res.waste;
        entry.source = convertToLeftoverSource(res.source);

        result.append(entry);
    }

    return result;
}

// 🔁 Egyedi CutResult → reusable darab
static inline LeftoverStockEntry toReusableEntry(const ResultModel& res) {
    LeftoverStockEntry entry;
    entry.materialId = res.materialId;
    entry.availableLength_mm = res.waste;
    entry.source = convertToLeftoverSource(res.source);;

    return entry;
}

}}}

