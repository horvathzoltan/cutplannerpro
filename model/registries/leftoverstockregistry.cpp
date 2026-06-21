#include "leftoverstockregistry.h"
#include <algorithm>
#include "../../common/filenamehelper.h"
#include "../repositories/leftoverstockrepository.h"

LeftoverStockRegistry &LeftoverStockRegistry::instance() {
    // 🧵 Singleton implementáció: egyetlen példány az egész programban
    static LeftoverStockRegistry reg;
    return reg;
}

void LeftoverStockRegistry::registerEntry(const LeftoverStockEntry& entry) {
    _data.append(entry);
    persist();
}

void LeftoverStockRegistry::clearAll() {
    _data.clear();
    persist(); // 💾 Mentés a törlés után
}

void LeftoverStockRegistry::persist() const {
    // if(!isPersist){
    //     return; // 🛑 Ha nem kell perzisztálni, akkor kilépünk
    // }
    const QString path = FileNameHelper::instance().getLeftoversCsvFile();
    if (!path.isEmpty())
        LeftoverStockRepository::saveToCSV(*this, path);
}

std::optional<LeftoverStockEntry> LeftoverStockRegistry::findByBarcode(const QString& barcode) const {
    for (const auto& entry : _data) {
        if (entry.barcode.toLower() == barcode.toLower())
            return entry;
    }
    return std::nullopt;
}



// bool LeftoverStockRegistry::removeByMaterialId(const QUuid& id) {
//     auto it = std::remove_if(_data.begin(), _data.end(), [&id](const LeftoverStockEntry& entry) {
//         return entry.materialId == id;
//     });
//     if (it != _data.end()) {
//         _data.erase(it, _data.end());
//         persist(); // 💾 Mentés csak akkor, ha történt törlés
//         return true;
//     }
//     return false;
// }

// QVector<LeftoverStockEntry> LeftoverStockRegistry::findByGroupName(const QString& name) const {
//     QVector<LeftoverStockEntry> result;
//     for (const auto& entry : _data) {
//         if (entry.materialGroupName() == name) {
//             result.append(entry);
//         }
//     }
//     return result;
// }

void LeftoverStockRegistry::consumeEntry(const QString& barcode)
{
    auto it = std::remove_if(_data.begin(), _data.end(),
                             [&](const LeftoverStockEntry& entry) {
                                 return entry.barcode == barcode;
                             });

    if (it != _data.end()) {
        _data.erase(it, _data.end()); // 🧹 Törlés a készletből
        persist(); // 💾 Mentés, ha tényleg töröltünk
    }
}


QVector<LeftoverStockEntry> LeftoverStockRegistry::filtered(int minLength_mm) const {
    QVector<LeftoverStockEntry> result;

    for (const auto& entry : _data) {
        if (entry.availableLength_mm >= minLength_mm)
            result.append(entry);
    }

    return result;
}

bool LeftoverStockRegistry::removeEntry(const QUuid& entryId) {
    auto it = std::remove_if(_data.begin(), _data.end(),
                             [&entryId](const LeftoverStockEntry& entry) {
                                 return entry.entryId == entryId;
                             });

    if (it != _data.end()) {
        _data.erase(it, _data.end());
        persist(); // 💾 Csak akkor mentjük, ha ténylegesen történt törlés
        return true;
    }
    return false;
}
bool LeftoverStockRegistry::updateEntry(const LeftoverStockEntry& updatedEntry) {
    for (auto& entry : _data) {
        if (entry.entryId == updatedEntry.entryId) {
            entry = updatedEntry;
            persist(); // 💾 Mentés a módosítás után
            return true;
        }
    }
    return false;
}

std::optional<LeftoverStockEntry> LeftoverStockRegistry::findById(const QUuid& entryId) const {
    for (const auto& entry : _data) {
        if (entry.entryId == entryId)
            return entry;
    }
    return std::nullopt;
}

bool LeftoverStockRegistry::existsBarcode(const QString& barcode,
                                          const QUuid& ignoreId) const
{
    QString bc = barcode.trimmed();

    for (const auto& entry : _data) {
        if (entry.entryId == ignoreId)
            continue; // szerkesztésnél ne önmagával ütközzön

        if (entry.barcode.compare(bc, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}


