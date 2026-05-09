#pragma once

#include <optional>
#include <QSet>
#include <QVector>
#include <QUuid>

#include "../../stockentry.h"
#include "selectedrod.h"

namespace Cutting {
namespace Optimizer {

class StockFitEngine {
public:
    static std::optional<SelectedRod> pickStockRod(
        QVector<StockEntry>& stockInventory,
        const QSet<QUuid>& groupIds,
        int& rodCounter);
};

} // namespace Optimizer
} // namespace Cutting
