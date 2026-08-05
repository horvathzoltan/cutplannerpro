#pragma once

#include <optional>
#include <QSet>
#include <QVector>
#include <QUuid>

#include "stock/model/stockentry.h"
//#include "model/cutting/piece/piecewithmaterial.h"
#include "selectedrod.h"

namespace Cutting {
namespace Optimizer {

class StockFitEngine {

private:
    static bool _isVerbose;
public:
    static std::optional<SelectedRod> pickStockRod(
        QVector<StockEntry>& stockInventory,
        const QSet<QUuid>& groupIds,
        int& rodCounter);

    static std::optional<SelectedRod> pickStockRod2(
        QVector<StockEntry>& stockInventory,
        const QSet<QUuid>& groupIds,
        int& rodCounter,
        int requestedLength_mm,
        double kerf_mm);
};

} // namespace Optimizer
} // namespace Cutting
