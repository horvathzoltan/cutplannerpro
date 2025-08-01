#include "reusablestockregistry.h"
#include <algorithm>
#include <common/filenamehelper.h>
#include <model/repositories/reusablestockrepository.h>


void ReusableStockRegistry::add(const ReusableStockEntry& entry) {
    _stock.append(entry);
}

void ReusableStockRegistry::clear() {
    _stock.clear();
    persist(); // 💾 Mentés a törlés után
}

void ReusableStockRegistry::persist() const {
    const QString path = FileNameHelper::instance().getLeftoversCsvFile();
    if (!path.isEmpty())
        ReusableStockRepository::saveToCSV(*this);
}


bool ReusableStockRegistry::removeByMaterialId(const QUuid& id) {
    auto it = std::remove_if(_stock.begin(), _stock.end(), [&id](const ReusableStockEntry& entry) {
        return entry.materialId == id;
    });
    if (it != _stock.end()) {
        _stock.erase(it, _stock.end());
        persist(); // 💾 Mentés csak akkor, ha történt törlés
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
        persist(); // 💾 Mentés, ha tényleg töröltünk
    }
}


QVector<ReusableStockEntry> ReusableStockRegistry::filtered(int minLength_mm) const {
    QVector<ReusableStockEntry> result;

    for (const auto& entry : _stock) {
        if (entry.availableLength_mm >= minLength_mm)
            result.append(entry);
    }

    return result;
}
