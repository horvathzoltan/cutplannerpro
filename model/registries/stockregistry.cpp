#include "stockregistry.h"
//#include <ranges>
#include <common/filenamehelper.h>
#include <model/repositories/stockrepository.h>

StockRegistry &StockRegistry::instance() {
    static StockRegistry reg;
    return reg;
}

void StockRegistry::registerEntry(const StockEntry& entry) {
    _data.append(entry);
    persist(); // 💾 Automatikus mentés
}

void StockRegistry::clearAll() {
    _data.clear();
    persist(); // 💾 Állapot mentés törlés után
}

// bool StockRegistry::removeByMaterialId(const QUuid& id) {
//     const int index = std::ranges::find_if(_data, [&](const auto& s) { return s.materialId == id; }) - _data.begin();
//     if (index >= 0 && index < _data.size()) {
//         _data.removeEntry(index);
//         persist(); // 💾 Mentés csak ha történt törlés
//         return true;
//     }
//     return false;
// }
void StockRegistry::removeEntry(const QUuid& id) {
    // 🗑️ Törlés egyedi azonosító alapján
    auto it = std::remove_if(_data.begin(), _data.end(),
                             [&](const StockEntry& r) {
                                 return r.entryId == id;
                             });

    if (it != _data.end()) {
        _data.erase(it, _data.end());
        persist(); // 💾 Mentés csak akkor, ha történt törlés
    }
}


QVector<StockEntry> StockRegistry::findByGroupName(const QString& name) const {
    QVector<StockEntry> result;
    for (const auto& entry : _data) {
        if (entry.materialGroupName() == name)
            result.append(entry);
    }
    return result;
}

void StockRegistry::persist() const {
    // if(!isPersist){
    //     return; // 🛑 Ha nem kell perzisztálni, akkor kilépünk
    // }
    const QString path = FileNameHelper::instance().getStockCsvFile(); // 🔧 Fix útvonal
    if (!path.isEmpty())
        StockRepository::saveToCSV(*this, path);
}



void StockRegistry::consumeEntry(const QUuid& materialId)
{
    for (StockEntry& entry : _data) {
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
    for (const auto& r : _data) {
        if (r.entryId == entryId)
            return r;
    }
    return std::nullopt;
}

bool StockRegistry::updateEntry(const StockEntry& updated) {
    // 🔍 Érvényesség ellenőrzése
    //if (!updated.isValid())
    //    return false;

    // 🔄 Megkeressük a megfelelő requestId-t a vektorban
    for (auto& r : _data) {
        if (r.entryId == updated.entryId) {
            r = updated; // ✏️ Frissítés
            persist();   // 💾 Mentés
            return true;
        }
    }

    // ❌ Nem találtuk meg az adott requestId-t
    return false;
}

QVector<StockEntry> StockRegistry::findByMaterialId(const QUuid& materialId) const {
    QVector<StockEntry> result;
    for (const auto& entry : _data) {
        if (entry.materialId == materialId) {
            result.append(entry);
        }
    }
    return result;
}
