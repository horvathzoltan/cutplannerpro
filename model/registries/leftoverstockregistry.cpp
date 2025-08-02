#include "leftoverstockregistry.h"
#include <algorithm>
#include <common/filenamehelper.h>
#include <model/repositories/leftoverstockrepository.h>


void LeftoverStockRegistry::add(const LeftoverStockEntry& entry) {
    _stock.append(entry);
}

void LeftoverStockRegistry::clear() {
    _stock.clear();
    persist(); // 💾 Mentés a törlés után
}

void LeftoverStockRegistry::persist() const {
    const QString path = FileNameHelper::instance().getLeftoversCsvFile();
    if (!path.isEmpty())
        LeftoverStockRepository::saveToCSV(*this);
}


bool LeftoverStockRegistry::removeByMaterialId(const QUuid& id) {
    auto it = std::remove_if(_stock.begin(), _stock.end(), [&id](const LeftoverStockEntry& entry) {
        return entry.materialId == id;
    });
    if (it != _stock.end()) {
        _stock.erase(it, _stock.end());
        persist(); // 💾 Mentés csak akkor, ha történt törlés
        return true;
    }
    return false;
}

QVector<LeftoverStockEntry> LeftoverStockRegistry::findByGroupName(const QString& name) const {
    QVector<LeftoverStockEntry> result;
    for (const auto& entry : _stock) {
        if (entry.materialGroupName() == name) {
            result.append(entry);
        }
    }
    return result;
}

void LeftoverStockRegistry::consume(const QString& barcode)
{
    auto it = std::remove_if(_stock.begin(), _stock.end(),
                             [&](const LeftoverStockEntry& entry) {
                                 return entry.barcode == barcode;
                             });

    if (it != _stock.end()) {        
        _stock.erase(it, _stock.end()); // 🧹 Törlés a készletből
        persist(); // 💾 Mentés, ha tényleg töröltünk
    }
}


QVector<LeftoverStockEntry> LeftoverStockRegistry::filtered(int minLength_mm) const {
    QVector<LeftoverStockEntry> result;

    for (const auto& entry : _stock) {
        if (entry.availableLength_mm >= minLength_mm)
            result.append(entry);
    }

    return result;
}

bool LeftoverStockRegistry::removeByEntryId(const QUuid& entryId) {
    auto it = std::remove_if(_stock.begin(), _stock.end(),
                             [&entryId](const LeftoverStockEntry& entry) {
                                 return entry.entryId == entryId;
                             });

    if (it != _stock.end()) {
        _stock.erase(it, _stock.end());
        persist(); // 💾 Csak akkor mentjük, ha ténylegesen történt törlés
        return true;
    }
    return false;
}
bool LeftoverStockRegistry::update(const LeftoverStockEntry& updatedEntry) {
    for (auto& entry : _stock) {
        if (entry.entryId == updatedEntry.entryId) {
            entry = updatedEntry;
            persist(); // 💾 Mentés a módosítás után
            return true;
        }
    }
    return false;
}

std::optional<LeftoverStockEntry> LeftoverStockRegistry::findById(const QUuid& entryId) const {
    for (const auto& entry : _stock) {
        if (entry.entryId == entryId)
            return entry;
    }
    return std::nullopt;
}


