#pragma once
#include "model/cutresult.h"
#include "model/reusablestockentry.h"

namespace CutResultUtils
{

static inline QVector<ReusableStockEntry> toReusableEntries(const QVector<CutResult>& input) {
    QVector<ReusableStockEntry> result;

    for (const CutResult& res : input) {
        result.append({
            res.materialId,
            res.waste,          // 🔁 Ez lesz az availableLength_mm
            res.source          // 🛠️ Átveszi a forrást is
        });
    }

    return result;
}

}

