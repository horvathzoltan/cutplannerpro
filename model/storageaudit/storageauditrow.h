#pragma once

#include "model/stockentry.h"
#include <QString>
#include <QUuid>
#include <model/registries/stockregistry.h>

enum class AuditSourceType {
    Stock,
    Leftover
};

struct StorageAuditRow {
    QUuid rowId = QUuid::createUuid(); // CRUD miatt kell egy azonosító
    QUuid materialId; // vagy barcode, amit a MaterialRegistry tud kezelni
    QUuid stockEntryId;                    // 🔗 Kapcsolat a StockEntry-hez
    AuditSourceType sourceType = AuditSourceType::Stock;    

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
        auto s = StockRegistry::instance().findById(stockEntryId);
        return s ? s->storageName() : "—";
    }

    QString status() const {
        if (sourceType == AuditSourceType::Leftover)
            return actualQuantity > 0 ? "OK" : "Hiányzik";

        if (pickingQuantity > 0) {
            return actualQuantity < pickingQuantity ? "Hiányzik" : "OK";
        }
        return "-";
    }
};
