#include "stockregistry.h"
#include <ranges>

void StockRegistry::add(const StockEntry& entry) {
    _stock.append(entry);
}

void StockRegistry::clear() {
    _stock.clear();
}

bool StockRegistry::removeByMaterialId(const QUuid& id) {
    const int index = std::ranges::find_if(_stock, [&](const auto& s) { return s.materialId == id; }) - _stock.begin();
    if (index >= 0 && index < _stock.size()) {
        _stock.remove(index);
        return true;
    }
    return false;
}

QVector<StockEntry> StockRegistry::findByGroupName(const QString& name) const {
    QVector<StockEntry> result;
    for (const auto& entry : _stock) {
        if (entry.groupName() == name)
            result.append(entry);
    }
    return result;
}

// QVector<StockEntry> StockRepository::(ProfileCategory cat) const {
//     QVector<StockEntry> result;
//     for (const auto& s : _stock)
//         if (s.category() == cat)
//             result.append(s);
//     return result;
// }
