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
    //bool isPresent = false;
    bool isInOptimization = false;

    QString barcode;
    QString storageName;

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

    // QString storageName() const {
    //     if (sourceType == AuditSourceType::Leftover) {
    //         const std::optional<LeftoverStockEntry> entry =
    //             LeftoverStockRegistry::instance().findById(stockEntryId);
    //         return entry ? entry->storageName() : "—";
    //     }

    //     const auto stock = StockRegistry::instance().findById(stockEntryId);
    //     return stock ? stock->storageName() : "—";
    // }

    // QString status() const {
    //     if (sourceType == AuditSourceType::Leftover) {
    //         if (actualQuantity == 0) {
    //             if (isInOptimization)
    //                 return "Felhasználás alatt, nincs megerősítve";
    //             else
    //                 return "Ellenőrzésre vár";
    //         } else {
    //             return "OK";
    //         }
    //     }

    //     switch (presence) {
    //     case AuditPresence::Present: return "OK";
    //     case AuditPresence::Missing: return "Hiányzik";
    //     case AuditPresence::Unknown: return "Ellenőrzésre vár";
    //     }
    //     return "-";
    // }

    QString status() const {
        // 🔍 Hulló audit esetén
        if (sourceType == AuditSourceType::Leftover) {
            if (isInOptimization) {
                if (actualQuantity > 0)
                    return "Felhasználás alatt, OK";
                else
                    return "Felhasználás alatt, nincs megerősítve";
            } else {
                return "Regisztrált hulló"; // nincs elvárt → semleges státusz
            }
        }

        // 📦 Stock audit esetén
        if (pickingQuantity == 0) {
            // nincs elvárt mennyiség → nincs viszonyítási alap
            return "Regisztrált készlet"; // semleges státusz
        }

        // 🎯 Ha van elvárt mennyiség, akkor audit státusz értelmezhető
        switch (presence) {
        case AuditPresence::Present:
            return "OK";
        case AuditPresence::Missing:
            return QString("Hiányzó mennyiség: %1").arg(pickingQuantity - actualQuantity);
        case AuditPresence::Unknown:
            return "Ellenőrzésre vár";
        }

        return "-";
    }


};
