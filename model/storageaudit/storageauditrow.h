#pragma once

#include "model/stockentry.h"
#include <QString>
#include <QUuid>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/stockregistry.h>
#include "model/storageaudit/audit_enums.h"
#include "model/storageaudit/auditcontext.h"
#include "model/storageaudit/auditstatus.h"
//#include "model/storageaudit/auditstatus_text.h"


struct StorageAuditRow {
    QUuid rowId = QUuid::createUuid(); // Egyedi azonosító (CRUD műveletekhez is kell)
    QUuid materialId;                  // Anyag azonosító (UUID)
    QUuid stockEntryId;                // Kapcsolat a StockEntry-hez
    AuditSourceType sourceType = AuditSourceType::Stock;
    //AuditPresence rowPresence = AuditPresence::Unknown;

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

    bool isFulfilled() const {
        return actualQuantity >= pickingQuantity;
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


    AuditStatus status() const {
        if (!isAudited()) {
            return (sourceType == AuditSourceType::Leftover)
                       ? AuditStatus(AuditStatus::RegisteredOnly)
                       : AuditStatus(AuditStatus::NotAudited);
        }
        return isFulfilled()
                   ? AuditStatus(AuditStatus::Audited_Fulfilled)
                   : AuditStatus(AuditStatus::Audited_Missing);
    }


    AuditStatus statusType() const {
        // 🔹 Hulló sor – mindig saját státusz alapján
        if (sourceType == AuditSourceType::Leftover) {
            return status();//AuditStatus::fromRow(isAudited(), isFulfilled(), true);
        }

        // 🔹 Egyedi sor – nincs csoport vagy csak 1 elem
        if (!context || context->group.size() <= 1) {
            return status();//AuditStatus::fromRow(isAudited(), isFulfilled());
        }

        // 🔹 Csoportos sor – a csoport auditáltsága számít
        return context->status();//AuditStatus::fromGroup(*context);
    }

    QString suffixForRow() const {
        if (sourceType == AuditSourceType::Leftover)
            return isAudited() ? (isFulfilled() ? AuditStatus::suffix_HulloVan() : AuditStatus::suffix_HulloNincs())
                               : AuditStatus::suffix_HulloNemAudit();

        if (!context || context->group.size() <= 1) {
            if (isRowModified)
                return isFulfilled() ? AuditStatus::suffix_Modositva() : AuditStatus::suffix_ModositvaNincs();
            if (isRowAuditChecked && !isFulfilled())
                return AuditStatus::suffix_NincsKeszlet();
        }

        return QString(); // nincs suffix
    }

    QString statusText() const {
        const AuditStatus s = statusType();
        const QString suffix = suffixForRow();
        return suffix.isEmpty() ? s.toDecoratedText() : AuditStatus::withSuffix(s.get(), suffix);
    }


    // QString statusText() const {
    //     const AuditStatus s = statusType();

    //     // 🔹 Leftover sorok
    //     if (sourceType == AuditSourceType::Leftover) {
    //         if (!isAudited()) {
    //             return AuditStatus::withSuffix(s.get(), AuditStatus::suffix_HulloNemAudit());
    //         }
    //         return isFulfilled()
    //                    ? AuditStatus::withSuffix(s.get(), AuditStatus::suffix_HulloVan())
    //                    : AuditStatus::withSuffix(s.get(), AuditStatus::suffix_HulloNincs());
    //     }

    //     // 🔹 Egyedi sor
    //     if (!context || context->group.size() <= 1) {
    //         if (isRowModified) {
    //             return isFulfilled()
    //                 ? AuditStatus::withSuffix(s.get(), AuditStatus::suffix_Modositva())
    //                 : AuditStatus::withSuffix(s.get(), AuditStatus::suffix_ModositvaNincs());
    //         }

    //         if (isRowAuditChecked) {
    //             return isFulfilled()
    //             ? s.toDecoratedText()
    //                        : AuditStatus::withSuffix(s.get(), AuditStatus::suffix_NincsKeszlet());
    //         }

    //         return s.toDecoratedText();
    //     }

    //     // 🔹 Csoportos sor
    //     return context->statusText();
    // }


};

