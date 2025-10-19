#pragma once

#include "model/stockentry.h"
#include <QString>
#include <QUuid>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/stockregistry.h>
#include "model/storageaudit/auditcontext.h"
#include "model/storageaudit/auditstatus.h"
//#include "model/storageaudit/auditstatus_text.h"


struct StorageAuditRow {
    QUuid rowId = QUuid::createUuid(); // Egyedi azonosító (CRUD műveletekhez is kell)
    QUuid materialId;                  // Anyag azonosító (UUID)
    QUuid stockEntryId;                // Kapcsolat a StockEntry-hez
    AuditSourceType sourceType = AuditSourceType::Stock;
    AuditPresence rowPresence = AuditPresence::Unknown;

    int pickingQuantity = 0;       // Elvárt mennyiség (soronként, injektálás után)
    //int actualQuantity = 0;        // Audit során talált mennyiség
    bool isInOptimization = false; // Része-e az optimalizációnak

    int originalQuantity;
    int actualQuantity;

    bool isRowModified = false; // sor szintű Módosult-e az eredeti értékhez képest
    bool isRowAuditChecked = false; // sor szintű pipa állapota
    AuditResult rowAuditResult; // sor szintű Audit állapot

    QString barcode;               // Vonalkód (ha van)
    QString storageName;           // Tároló neve
    QUuid rootStorageId; // 🆕 Gépszintű csoportosításhoz szükséges

    // Kontextus pointer: azonos anyag+hely csoport összesített adatai
    std::shared_ptr<AuditContext> context;

    /**
 * @brief Hiányzó mennyiség számítása sor szinten.
 *
 * - Ha van hozzárendelt AuditContext, akkor a csoport szintű hiányt adja vissza
 *   (delegál a AuditContext::missingQuantity()-ra).
 * - Ha nincs AuditContext, akkor a sor saját pickingQuantity és actualQuantity
 *   mezői alapján számolja a hiányt.
 *
 * @note A hiány sosem lehet negatív. A cél az audit teljesülésének ellenőrzése,
 *       nem a többlet kimutatása.
 *
 * @return int A hiányzó mennyiség (sor vagy csoport szinten).
 */
    [[nodiscard]] int missingQuantity() const noexcept {
        if (context) return context->missingQuantity();
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
        // 🔹 Ha a felhasználó auditáltnak jelölte (pipa)
        if (isRowAuditChecked) {
            if (actualQuantity > 0)
                return "Auditált, OK";
            else
                return "Auditált, nincs készlet";
        }

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
        switch (rowPresence) {
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
    // AuditStatus statusType() const {
    //     // Ha nem része az optimalizációnak → csak információ
    //     if (!isInOptimization) {
    //         return AuditStatus::Info;
    //     }

    //     // Ha nincs context, fallback a lokális mezőkre
    //     if (!context) {
    //         if (pickingQuantity == 0 && actualQuantity > 0)
    //             return AuditStatus::Info;
    //         if (pickingQuantity == 0)
    //             return AuditStatus::Info;
    //         if (actualQuantity == 0)
    //             return AuditStatus::Missing;
    //         if (actualQuantity < pickingQuantity)
    //             return AuditStatus::Pending;
    //         if (actualQuantity >= pickingQuantity)
    //             return AuditStatus::Ok;
    //         return AuditStatus::Unknown;
    //     }

    //     // Kontextus szerinti értékelés (anyag+hely csoport szinten)
    //     const int expected = context->totalExpected;
    //     const int actual   = context->totalActual;

    //     if (expected == 0 && actual > 0) return AuditStatus::Info;
    //     if (expected == 0 && actual == 0) return AuditStatus::Info;
    //     if (actual == 0) return AuditStatus::Missing;
    //     if (actual < expected) return AuditStatus::Pending;
    //     if (actual >= expected) return AuditStatus::Ok;

    //     return AuditStatus::Unknown;
    // }

    // AuditStatus statusType() const {
    //     // 🔹 Ha a felhasználó explicit auditálta (pipa), akkor ez az elsődleges
    //     if (isAuditConfirmed) {
    //         if (actualQuantity > 0)
    //             return AuditStatus::Ok;       // Auditált és van készlet
    //         else
    //             return AuditStatus::Missing;  // Auditált, de nincs készlet
    //     }

    //     // ha nincs audit megerősítve, akkor az alábbi logika jön
    //     // 🔹 Leftover audit külön logika
    //     if (sourceType == AuditSourceType::Leftover && isInOptimization) {
    //         switch (presence) {
    //         case AuditPresence::Present:   return AuditStatus::Ok;
    //         case AuditPresence::Missing:   return AuditStatus::Missing;
    //         case AuditPresence::Unknown:   return AuditStatus::Pending;
    //         }
    //         return AuditStatus::Unknown;
    //     }

    //     // 🔸 Nem optimalizált → csak információ
    //     if (!isInOptimization) {
    //         return AuditStatus::Info;
    //     }

    //     // 🔸 Lokális fallback
    //     if (!context) {
    //         if (pickingQuantity == 0 && actualQuantity > 0)
    //             return AuditStatus::Info;
    //         if (pickingQuantity == 0)
    //             return AuditStatus::Info;
    //         if (actualQuantity == 0)
    //             return AuditStatus::Missing;
    //         if (actualQuantity < pickingQuantity)
    //             return AuditStatus::Pending;
    //         if (actualQuantity >= pickingQuantity)
    //             return AuditStatus::Ok;
    //         return AuditStatus::Unknown;
    //     }

    //     // 🔸 Kontextus szerinti értékelés
    //     const int expected = context->totalExpected;
    //     const int actual   = context->totalActual;

    //     if (expected == 0 && actual > 0) return AuditStatus::Info;
    //     if (expected == 0 && actual == 0) return AuditStatus::Info;
    //     if (actual == 0) return AuditStatus::Missing;
    //     if (actual < expected) return AuditStatus::Pending;
    //     if (actual >= expected) return AuditStatus::Ok;

    //     return AuditStatus::Unknown;
    // }


    // // Szöveges státusz (konzisztens a statusType()-pal)
    // QString statusText() const {
    //     // 🔹 Ha nincs audit megerősítve → mindig "Nem auditált"
    //     if (!isAuditConfirmed) {
    //         return "⚪ Nem auditált";
    //     }

    //     // 🔹 Ha auditálva van, akkor jön a meglévő logika
    //     if (sourceType == AuditSourceType::Leftover && isInOptimization) {
    //         QString prefix = "Felhasználás alatt, ";

    //         switch (presence) {
    //         case AuditPresence::Present:
    //             return prefix + "OK";
    //         case AuditPresence::Missing:
    //             return prefix + "Hiányzik";
    //         case AuditPresence::Unknown:
    //             return prefix + "Ellenőrzésre vár";
    //         }
    //         return prefix + "-";
    //     }

    //     return StorageAudit::Status::toText(statusType());
    // }

    // AuditStatus statusType() const {
    //     // 🔹 Ha nincs audit megerősítve → Nem auditált
    //     if (!isRowAuditChecked) {
    //         return AuditStatus::Unknown; // vagy definiálhatsz külön NotAudited enumot
    //     }

    //     // 🔹 Leftover sorok külön logika
    //     if (sourceType == AuditSourceType::Leftover) {
    //         if (isInOptimization) {
    //             return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
    //         }
    //         return AuditStatus::Info; // regisztrált hulló
    //     }

    //     // 🔹 Ha nem része az optimalizációnak → csak információ
    //     if (!isInOptimization) {
    //         return AuditStatus::Info;
    //     }

    //     // 🔹 Ha nincs context → egyedi sor
    //     if (!context) {
    //         if (actualQuantity == 0) return AuditStatus::Missing;
    //         return AuditStatus::Ok; // egyedi sor auditált
    //     }

    //     // 🔹 Kontextus szerinti értékelés (anyagcsoport szinten)
    //     const int expected = context->totalExpected;
    //     const int actual   = context->totalActual;

    //     if (actual == 0) return AuditStatus::Missing;
    //     if (actual < expected) return AuditStatus::Pending; // részlegesen auditált
    //     if (actual >= expected) return AuditStatus::Ok;     // teljesen auditált

    //     return AuditStatus::Unknown;
    // }

    // QString statusText() const {
    //     if (sourceType == AuditSourceType::Leftover) {
    //         if (!isRowAuditChecked) {
    //             return "⚪ Nem auditált ♻️ (hulló)";
    //         }

    //         switch (rowPresence) {
    //         case AuditPresence::Present: return "🟢 Auditált ♻️ (van)";
    //         case AuditPresence::Missing: return "🟠 Auditált ♻️ (nincs)";
    //         case AuditPresence::Unknown: return "⚪ Nem auditált ♻️";
    //         }
    //         return "🟢 Auditált ♻️ (regisztrált hulló)";
    //     }

    //     if (context) {
    //         switch (context->presence()) {
    //         case AuditPresence::Unknown: return "⚪ Nem auditált";
    //         case AuditPresence::Missing: return "🟡 Részlegesen auditált";
    //         case AuditPresence::Present: return "🟢 Auditálva";
    //         }
    //     }

    //     // fallback ha nincs context
    //     return isRowAuditChecked ? "🟢 Auditálva" : "⚪ Nem auditált";
    // }



    AuditStatus statusType() const {
        // 🔹 Leftover sorok külön logika
        if (sourceType == AuditSourceType::Leftover) {
            if (isInOptimization) {
                return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
            }
            return AuditStatus::Info; // regisztrált hulló
        }

        // 🔹 Ha nincs audit pipa → nem auditált
        if (!isRowAuditChecked) {
            return AuditStatus::Unknown; // vagy külön NotAudited enum
        }

        // 🔹 Ha van context és a sor tényleg group tag
        if (context && context->group.size() > 1) {
            switch (context->groupPresence()) {
            case AuditPresence::Unknown: return AuditStatus::Unknown;
            case AuditPresence::Missing: return AuditStatus::Pending; // részlegesen auditált
            case AuditPresence::Present: return AuditStatus::Ok;      // teljesen auditált
            }
        }

        // 🔹 Ha nincs context → egyedi sor logika
        if (actualQuantity == 0) return AuditStatus::Missing;
        return AuditStatus::Ok;
    }

    QString statusText() const {
        // 🔹 Leftover sorok külön logika
        if (sourceType == AuditSourceType::Leftover) {
            if (!isRowAuditChecked) {
                return "⚪ Nem auditált ♻️ (hulló)";
            }
            return (actualQuantity > 0)
                       ? "🟢 Auditált ♻️ (van)"
                       : "🟠 Auditált ♻️ (nincs)";
        }

        // 🔹 Ha van context és a sor tényleg group tag
        if (context && context->group.size() > 1) {
            switch (context->groupPresence()) {
            case AuditPresence::Unknown: return "⚪ Nem auditált (csoport)";
            case AuditPresence::Missing: return "🟡 Részlegesen auditált (csoport)";
            case AuditPresence::Present: return "🟢 Auditálva (csoport)";
            }
        }

        // 🔹 Ha nincs context → egyedi sor logika
        if (isRowAuditChecked) {
            return (actualQuantity > 0)
            ? "🟢 Auditálva"
            : "🟠 Auditált, nincs készlet";
        }

        return "⚪ Nem auditált";
    }

};

