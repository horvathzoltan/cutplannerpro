#pragma once

#include "model/stockentry.h"
#include <QString>
#include <QUuid>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/stockregistry.h>

enum class AuditSourceType {
    Stock,
    Leftover
};

enum class AuditPresence {
    Unknown,
    Present,
    Missing
};


struct StorageAuditRow {
    QUuid rowId = QUuid::createUuid(); // CRUD miatt kell egy azonosító
    QUuid materialId; // vagy barcode, amit a MaterialRegistry tud kezelni
    QUuid stockEntryId;                    // 🔗 Kapcsolat a StockEntry-hez
    AuditSourceType sourceType = AuditSourceType::Stock;    
    AuditPresence presence = AuditPresence::Unknown;

    int pickingQuantity = 0;       // Elvárt mennyiség (picking alapján)
    int actualQuantity = 0;        // Audit során talált mennyiség
    bool isPresent = false;

    // int missingQuantity() const {
    //     if(pickingQuantity==0) return 0; // ha nincs picking quantity, nincs elvárt quantity sem
    //     if(pickingQuantity < actualQuantity) return 0; // ha több van ott,mint kell, nincs hiány

    //     return pickingQuantity - actualQuantity; // hiányzó
    // }
    int missingQuantity() const {
        return (pickingQuantity > actualQuantity) ? (pickingQuantity - actualQuantity) : 0;
    }

    QUuid storageId() const {
        std::optional<StockEntry> s =
            StockRegistry::instance().findById(stockEntryId);
        if(!s.has_value()) return QUuid();

        return s.value().storageId;
    }

    QString storageName() const {
        if (sourceType == AuditSourceType::Leftover) {
            const std::optional<LeftoverStockEntry> entry =
                LeftoverStockRegistry::instance().findById(stockEntryId);
            return entry ? entry->storageName() : "—";
        }

        const auto stock = StockRegistry::instance().findById(stockEntryId);
        return stock ? stock->storageName() : "—";
    }

    QString status() const {
        switch (presence) {
        case AuditPresence::Present: return "OK";
        case AuditPresence::Missing: return "Hiányzik";
        case AuditPresence::Unknown: return "Ellenőrzésre vár";
        }
        return "-";
    }

    // QString status() const {
    //     if (sourceType == AuditSourceType::Leftover) {
    //         const auto entry = LeftoverStockRegistry::instance().findById(stockEntryId);
    //         if (entry) {
    //             if (actualQuantity == 0) {
    //                 return "Ellenőrzésre vár"; // papíron ott van, de nincs megerősítve
    //             } else {
    //                 return "OK"; // megerősítve
    //             }
    //         } else {
    //             return "Nem szerepel"; // nincs nyilvántartva, de auditban megjelent
    //         }
    //     }

    //     if (pickingQuantity > 0) {
    //         return actualQuantity < pickingQuantity ? "Hiányzik" : "OK";
    //     }

    //     return "-";
    // }


};
