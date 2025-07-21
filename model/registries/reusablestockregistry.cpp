#include "reusablestockregistry.h"
#include <algorithm>


void ReusableStockRegistry::add(const ReusableStockEntry& entry) {
    _stock.append(entry);
}

void ReusableStockRegistry::clear() {
    _stock.clear();
}



bool ReusableStockRegistry::removeByMaterialId(const QUuid& id) {
    auto it = std::remove_if(_stock.begin(), _stock.end(), [&id](const ReusableStockEntry& entry) {
        return entry.materialId == id;
    });
    if (it != _stock.end()) {
        _stock.erase(it, _stock.end());
        return true;
    }
    return false;
}

QVector<ReusableStockEntry> ReusableStockRegistry::findByGroupName(const QString& name) const {
    QVector<ReusableStockEntry> result;
    for (const auto& entry : _stock) {
        if (entry.groupName() == name) {
            result.append(entry);
        }
    }
    return result;
}

void ReusableStockRegistry::consume(const QString& barcode)
{
    auto it = std::remove_if(_stock.begin(), _stock.end(),
                             [&](const ReusableStockEntry& entry) {
                                 return entry.barcode == barcode;
                             });

    if (it != _stock.end()) {
        _stock.erase(it, _stock.end()); // 🧹 Törlés a készletből
    }
}
