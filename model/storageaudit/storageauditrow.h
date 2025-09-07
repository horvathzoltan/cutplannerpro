#pragma once

#include "model/stockentry.h"
#include <QString>
#include <QUuid>
#include <model/registries/stockregistry.h>

struct StorageAuditRow {
    QUuid rowId = QUuid::createUuid(); // CRUD miatt kell egy azonosító
    QUuid materialId; // vagy barcode, amit a MaterialRegistry tud kezelni
    QUuid stockEntryId;                    // 🔗 Kapcsolat a StockEntry-hez
    //QString storageName;
    //QString materialName;
    //QString materialBarcode;
    //QString colorCode;

    int pickingQuantity = 0;       // Elvárt mennyiség (picking alapján)
    int actualQuantity = 0;        // Audit során talált mennyiség
    bool isPresent = false;

    int missingQuantity() const {
        if(pickingQuantity==0) return 0; // ha nincs picking quantity, nincs elvárt quantity sem
        if(pickingQuantity < actualQuantity) return 0; // ha több van ott,mint kell, nincs hiány

        return pickingQuantity - actualQuantity; // hiányzó
    }

    QString storageName() const {
        std::optional<StockEntry> s =
            StockRegistry::instance().findById(stockEntryId);
        if(!s.has_value()) return QString("-");

        return s.value().storageName();
    }

    QString status() const {
        if(pickingQuantity>0){
            //if (actualQuantity == pickingQuantity) return "OK";
            if (actualQuantity < pickingQuantity) return "Hiányzik";

            return "OK";
        } else{
            return "-";
        }
    }
};
