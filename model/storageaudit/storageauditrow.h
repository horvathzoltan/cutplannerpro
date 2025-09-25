#pragma once

#include "model/stockentry.h"
#include <QString>
#include <QUuid>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/stockregistry.h>
#include "model/storageaudit/auditcontext.h"
#include "model/storageaudit/auditstatus.h"
#include "model/storageaudit/auditstatus_text.h"

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
    QUuid rowId = QUuid::createUuid(); // Egyedi azonosító (CRUD műveletekhez is kell)
    QUuid materialId;                  // Anyag azonosító (UUID)
    QUuid stockEntryId;                // Kapcsolat a StockEntry-hez
    AuditSourceType sourceType = AuditSourceType::Stock;
    AuditPresence presence = AuditPresence::Unknown;

    int pickingQuantity = 0;       // Elvárt mennyiség (soronként, injektálás után)
    int actualQuantity = 0;        // Audit során talált mennyiség
    bool isInOptimization = false; // Része-e az optimalizációnak

    QString barcode;               // Vonalkód (ha van)
    QString storageName;           // Tároló neve
    QUuid rootStorageId; // 🆕 Gépszintű csoportosításhoz szükséges

    // Kontextus pointer: azonos anyag+hely csoport összesített adatai
    std::shared_ptr<AuditContext> context;

    // Hiányzó mennyiség számítása
    // ⚠️ Fontos: ha van context, akkor az összesített értékekből számolunk,
    // nem a sor lokális pickingQuantity-jából.
    // 🧠 A hiány sosem lehet negatív — auditálás célja a teljesülés ellenőrzése, nem a többlet kimutatása.
    int missingQuantity() const {
        if (context) {
            int expected = context->totalExpected;
            int actual   = context->totalActual;

            // 🔒 Védjük a negatív érték ellen
            return std::max(0, expected - actual);
        }

        // 🔹 Egyedi sor esetén: lokális hiány, de szintén védve
        return std::max(0, pickingQuantity - actualQuantity);
    }


    // Tároló UUID lekérése a StockRegistry-ből
    QUuid storageId() const {
        std::optional<StockEntry> s =
            StockRegistry::instance().findById(stockEntryId);
        if(!s.has_value()) return QUuid();
        return s.value().storageId;
    }

    // Szöveges státusz (UI-hoz)
    QString status() const {
        // Hulló audit esetén külön logika
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

        // Stock audit esetén
        if (pickingQuantity == 0) {
            // nincs elvárt mennyiség → nincs viszonyítási alap
            return "Regisztrált készlet";
        }

        // Ha van elvárt mennyiség, akkor audit státusz értelmezhető
        switch (presence) {
        case AuditPresence::Present:
            return "OK";
        case AuditPresence::Missing:
            return QString("Hiányzó mennyiség: %1").arg(missingQuantity());
        case AuditPresence::Unknown:
            return "Ellenőrzésre vár";
        }
        return "-";
    }

    // Audit státusz típus (ikonhoz, színezéshez)
    AuditStatus statusType() const {
        // Ha nem része az optimalizációnak → csak információ
        if (!isInOptimization) {
            return AuditStatus::Info;
        }

        // Ha nincs context, fallback a lokális mezőkre
        if (!context) {
            if (pickingQuantity == 0 && actualQuantity > 0)
                return AuditStatus::Info;
            if (pickingQuantity == 0)
                return AuditStatus::Info;
            if (actualQuantity == 0)
                return AuditStatus::Missing;
            if (actualQuantity < pickingQuantity)
                return AuditStatus::Pending;
            if (actualQuantity >= pickingQuantity)
                return AuditStatus::Ok;
            return AuditStatus::Unknown;
        }

        // Kontextus szerinti értékelés (anyag+hely csoport szinten)
        const int expected = context->totalExpected;
        const int actual   = context->totalActual;

        if (expected == 0 && actual > 0) return AuditStatus::Info;
        if (expected == 0 && actual == 0) return AuditStatus::Info;
        if (actual == 0) return AuditStatus::Missing;
        if (actual < expected) return AuditStatus::Pending;
        if (actual >= expected) return AuditStatus::Ok;

        return AuditStatus::Unknown;
    }

    // Szöveges státusz (konzisztens a statusType()-pal)
    QString statusText() const {
        if (sourceType == AuditSourceType::Leftover && isInOptimization) {
            if (actualQuantity > 0) return "Felhasználás alatt, OK";
        }
        return StorageAudit::Status::toText(statusType());
    }
};

