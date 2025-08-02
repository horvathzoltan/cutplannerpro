#include "stockregistry.h"
//#include <ranges>
#include <common/filenamehelper.h>
#include <model/repositories/stockrepository.h>

void StockRegistry::add(const StockEntry& entry) {
    _stock.append(entry);
    persist(); // 💾 Automatikus mentés
}

void StockRegistry::clear() {
    _stock.clear();
    persist(); // 💾 Állapot mentés törlés után
}

// bool StockRegistry::removeByMaterialId(const QUuid& id) {
//     const int index = std::ranges::find_if(_stock, [&](const auto& s) { return s.materialId == id; }) - _stock.begin();
//     if (index >= 0 && index < _stock.size()) {
//         _stock.remove(index);
//         persist(); // 💾 Mentés csak ha történt törlés
//         return true;
//     }
//     return false;
// }
void StockRegistry::remove(const QUuid& id) {
    // 🗑️ Törlés egyedi azonosító alapján
    auto it = std::remove_if(_stock.begin(), _stock.end(),
                             [&](const StockEntry& r) {
                                 return r.entryId == id;
                             });

    if (it != _stock.end()) {
        _stock.erase(it, _stock.end());
        persist(); // 💾 Mentés csak akkor, ha történt törlés
    }
}


QVector<StockEntry> StockRegistry::findByGroupName(const QString& name) const {
    QVector<StockEntry> result;
    for (const auto& entry : _stock) {
        if (entry.materialGroupName() == name)
            result.append(entry);
    }
    return result;
}

void StockRegistry::persist() const {
    const QString path = FileNameHelper::instance().getStockCsvFile(); // 🔧 Fix útvonal
    if (!path.isEmpty())
        StockRepository::saveToCSV(*this, path);
}

void StockRegistry::consume(const QUuid& materialId)
{
    for (StockEntry& entry : _stock) {
        if (entry.materialId == materialId) {
            if (entry.quantity > 0) {
                entry.quantity -= 1; // 🧮 Levonunk egy darabot
                persist(); // 💾 Készletváltozás mentése
            }
            break; // ✅ Csak az első egyezőre reagálunk
        }
    }
}

std::optional<StockEntry> StockRegistry::findById(const QUuid& entryId) const {
    for (const auto& r : _stock) {
        if (r.entryId == entryId)
            return r;
    }
    return std::nullopt;
}

bool StockRegistry::update(const StockEntry& updated) {
    // 🔍 Érvényesség ellenőrzése
    //if (!updated.isValid())
    //    return false;

    // 🔄 Megkeressük a megfelelő requestId-t a vektorban
    for (auto& r : _stock) {
        if (r.entryId == updated.entryId) {
            r = updated; // ✏️ Frissítés
            persist();   // 💾 Mentés
            return true;
        }
    }

    // ❌ Nem találtuk meg az adott requestId-t
    return false;
}
