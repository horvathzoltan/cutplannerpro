#pragma once
#include "model/cutting/result/resultmodel.h"
#include "model/cutting/result/resultsource.h"
#include "model/cutting/result/leftoversource.h"
#include "model/leftoverstockentry.h"

namespace Cutting{
namespace Result{

namespace ResultUtils
{

static inline LeftoverSource convertToLeftoverSource(ResultSource source) {
    switch (source) {
    case ResultSource::FromStock: return LeftoverSource::Optimization;
    case ResultSource::FromReusable: return LeftoverSource::Manual;
    case ResultSource::Unknown:
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

