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
    //AuditPresence rowPresence = AuditPresence::Unknown;

    bool isFulfilled() const {
        return actualQuantity >= pickingQuantity;
    }

    int pickingQuantity = 0;       // Elvárt mennyiség (soronként, injektálás után)
    //int actualQuantity = 0;        // Audit során talált mennyiség
    bool isInOptimization = false; // Része-e az optimalizációnak

    int originalQuantity;
    int actualQuantity;

    bool isRowModified = false; // sor szintű Módosult-e az eredeti értékhez képest
    bool isRowAuditChecked = false; // sor szintű pipa állapota
    //AuditResult rowAuditResult; // sor szintű Audit állapot

    bool isAudited() const {
        return isRowModified || isRowAuditChecked;
    }

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
    // QString status() const {
    //     // 🔹 Ha a felhasználó auditáltnak jelölte (pipa)
    //     if (isRowAuditChecked) {
    //         if (actualQuantity > 0)
    //             return "Auditált, OK";
    //         else
    //             return "Auditált, nincs készlet";
    //     }

    //     // Hulló audit esetén külön logika
    //     if (sourceType == AuditSourceType::Leftover) {
    //         if (isInOptimization) {
    //             if (actualQuantity > 0)
    //                 return "Felhasználás alatt, OK";
    //             else
    //                 return "Felhasználás alatt, nincs megerősítve";
    //         } else {
    //             return "Regisztrált hulló"; // nincs elvárt → semleges státusz
    //         }
    //     }

    //     // Stock audit esetén
    //     if (pickingQuantity == 0) {
    //         // nincs elvárt mennyiség → nincs viszonyítási alap
    //         return "Regisztrált készlet";
    //     }

    //     // Ha van elvárt mennyiség, akkor audit státusz értelmezhető
    //     switch (rowPresence) {
    //     case AuditPresence::Present:
    //         return "OK";
    //     case AuditPresence::Missing:
    //         return QString("Hiányzó mennyiség: %1").arg(missingQuantity());
    //     case AuditPresence::Unknown:
    //         return "Ellenőrzésre vár";
    //     }
    //     return "-";
    // }


    // AuditStatus statusType() const {
    //     // 🔹 Leftover sorok külön logika
    //     if (sourceType == AuditSourceType::Leftover) {
    //         if (isInOptimization) {
    //             return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
    //         }
    //         return AuditStatus::Info; // regisztrált hulló
    //     }

    //     // 🔹 Ha módosult → auditáltként kezeljük
    //     if (isRowModified) {
    //         return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
    //     }

    //     // 🔹 Ha van context és a sor tényleg group tag
    //     if (context && context->group.size() > 1) {
    //         switch (context->groupPresence()) {
    //         case AuditPresence::Unknown: return AuditStatus::Unknown;
    //         case AuditPresence::Missing: return AuditStatus::Pending;
    //         case AuditPresence::Present: return AuditStatus::Ok;
    //         }
    //     }

    //     // 🔹 Ha nincs context → egyedi sor logika
    //     if (isRowAuditChecked) {
    //         return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
    //     }

    //     return AuditStatus::Unknown;
    // }

    // QString statusText() const {
    //     // 🔹 Leftover sorok külön logika
    //     if (sourceType == AuditSourceType::Leftover) {
    //         if (!isRowAuditChecked) {
    //             return "⚪ Nem auditált ♻️ (hulló)";
    //         }
    //         return (actualQuantity > 0)
    //                    ? "🟢 Auditált ♻️ (van)"
    //                    : "🟠 Auditált ♻️ (nincs)";
    //     }

    //     // 🔹 Ha módosult → auditáltként kezeljük
    //     if (isRowModified) {
    //         return (actualQuantity > 0)
    //         ? "🟢 Auditált (módosítva)"
    //         : "🟠 Auditált, nincs készlet (módosítva)";
    //     }

    //     // 🔹 Ha van context és a sor tényleg group tag
    //     if (context && context->group.size() > 1) {
    //         switch (context->groupPresence()) {
    //         case AuditPresence::Unknown: return "⚪ Nem auditált (csoport)";
    //         case AuditPresence::Missing: return "🟡 Részlegesen auditált (csoport)";
    //         case AuditPresence::Present: return "🟢 Auditálva (csoport)";
    //         }
    //     }

    //     // 🔹 Ha nincs context → egyedi sor logika
    //     if (isRowAuditChecked) {
    //         return (actualQuantity > 0)
    //         ? "🟢 Auditálva"
    //         : "🟠 Auditált, nincs készlet";
    //     }

    //     return "⚪ Nem auditált";
    // }

   //  AuditStatus statusType() const {
   //      // 🔹 Leftover sorok külön logika
   //      if (sourceType == AuditSourceType::Leftover) {
   //          if (isInOptimization) {
   //              return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
   //          }
   //          return AuditStatus::Info;
   //      }

   //      // 🔹 Ha módosult → auditáltként kezeljük
   //      if (isRowModified) {
   //          return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
   //      }

   //      // 🔹 Ha pipált → auditáltként kezeljük
   //      if (isRowAuditChecked) {
   //          return (actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing;
   //      }

   //      // 🔹 Ha van context és csoport tag
   //      if (context && context->group.size() > 1) {
   //          switch (context->groupPresence()) {
   //          case AuditPresence::Unknown: return AuditStatus::Unknown; // ⚪ semmi nincs auditálva
   //          case AuditPresence::Missing: return AuditStatus::Pending; // 🟡 részlegesen auditált
   //          case AuditPresence::Present: return AuditStatus::Ok;      // 🟢 minden auditált
   //          }
   //      }

   //      return AuditStatus::Unknown;
   //  }





   //  QString statusText() const {
   //      // 🔹 Leftover sorok külön logika
   //      if (sourceType == AuditSourceType::Leftover) {
   //          if (!isRowAuditChecked) {
   //              return "⚪ Nem auditált ♻️ (hulló)";
   //          }
   //          return (actualQuantity > 0)
   //                     ? "🟢 Auditált ♻️ (van)"
   //                     : "🟠 Auditált ♻️ (nincs)";
   //      }

   //      // 🔹 Ha módosult → auditáltként kezeljük
   //      if (isRowModified) {
   //          return (actualQuantity > 0)
   //          ? "🟢 Auditált (módosítva)"
   //          : "🟠 Auditált, nincs készlet (módosítva)";
   //      }

   //      // 🔹 Ha pipált → auditáltként kezeljük
   //      if (isRowAuditChecked) {
   //          return (actualQuantity > 0)
   //          ? "🟢 Auditálva"
   //          : "🟠 Auditált, nincs készlet";
   //      }

   //      // 🔹 Ha van context és csoport tag
   //      if (context && context->group.size() > 1) {
   //          switch (context->groupPresence()) {
   //          case AuditPresence::Unknown: return "⚪ Nem auditált (csoport)";
   //          case AuditPresence::Missing: return "🟡 Részlegesen auditált (csoport)";
   //          case AuditPresence::Present: return "🟢 Auditálva (csoport)";
   //          }
   //      }

   //      return "⚪ Nem auditált";
   //  }

    AuditStatus statusType() const {
        // 🔹 Leftover sorok külön logika
        if (sourceType == AuditSourceType::Leftover) {
            if (isInOptimization) {
                return AuditStatus((actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing);
            }
            return AuditStatus(AuditStatus::Info);
        }

        // 🔹 Sor szintű auditáltság (módosítás vagy pipa)
        if (isRowModified || isRowAuditChecked) {
            return AuditStatus((actualQuantity > 0) ? AuditStatus::Ok : AuditStatus::Missing);
        }

        // 🔹 Csoport szintű auditáltság
        if (context && context->group.size() > 1) {
            return AuditStatus::fromPresence(context->groupPresence());
        }

        // 🔹 Alapértelmezett
        return AuditStatus(AuditStatus::Unknown);
    }


    QString statusText() const {
        AuditStatus status = statusType();

        // 🔹 Leftover sorok külön jelöléssel
        if (sourceType == AuditSourceType::Leftover) {
            if (isRowModified || isRowAuditChecked) {
                return (actualQuantity > 0)
                ? AuditStatus::withSuffix(AuditStatus::Ok, AuditStatus::suffixHullóVan())
                : AuditStatus::withSuffix(AuditStatus::Missing, AuditStatus::suffixHullóNincs());
            }
            return AuditStatus::withSuffix(AuditStatus::Info, AuditStatus::suffixHullóNemAudit());
        }


        // 🔹 Módosított sor külön jelöléssel
        if (isRowModified) {
            return (actualQuantity > 0)
            ? AuditStatus::withSuffix(AuditStatus::Ok, AuditStatus::suffixMódosítva())
            : AuditStatus::withSuffix(AuditStatus::Missing, AuditStatus::suffixMódosítvaNincs());
        }

        // 🔹 Pipált sor külön jelöléssel
        if (isRowAuditChecked) {
            return (actualQuantity > 0)
            ? AuditStatus::toDecoratedText(AuditStatus::Ok)
            : AuditStatus::withSuffix(AuditStatus::Missing, AuditStatus::suffixNincsKészlet());
        }

        // 🔹 Csoportos sor → helperből
        if (context && context->group.size() > 1) {
            return AuditStatus::fromPresenceText(context->groupPresence());
        }

        // 🔹 Egyébként az alap státusz szövege
        return status.toDecoratedText();
    }





};

