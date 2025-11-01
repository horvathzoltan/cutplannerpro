#pragma once

#include "model/stockentry.h"
#include <QString>
#include <QUuid>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/stockregistry.h>
#include "model/storageaudit/audit_enums.h"
#include "model/storageaudit/auditcontext.h"
#include "model/storageaudit/auditstatus.h"
//#include "view/cellhelpers/auditstatustext.h"
//#include "model/storageaudit/auditstatus_text.h"


struct StorageAuditRow {
    QUuid rowId = QUuid::createUuid(); // Egyedi azonosító (CRUD műveletekhez is kell)
    QUuid materialId;                  // Anyag azonosító (UUID)
    QUuid stockEntryId;                // Kapcsolat a StockEntry-hez
    AuditSourceType sourceType = AuditSourceType::Stock;
    //AuditPresence rowPresence = AuditPresence::Unknown;

    //int pickingQuantity = 0;       // Elvárt mennyiség (soronként, injektálás után)
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

    // bool isFulfilled() const {
    //     return actualQuantity >= pickingQuantity;
    // }
    // Új
    bool isFulfilled() const noexcept {
        if (context) {
            return context->totalActual >= context->totalExpected;
        }
        // Nincs context → nincs elvárás → nem tekintjük hibának
        return true;
    }

    QString barcode;               // Vonalkód (ha van)
    QString storageName;           // Tároló neve
    QUuid rootStorageId; // 🆕 Gépszintű csoportosításhoz szükséges

private:
    // Kontextus pointer: azonos anyag+hely csoport összesített adatai
    std::shared_ptr<AuditContext> context;
public:
    void setContext(const std::shared_ptr<AuditContext>& ctx) { context = ctx; }
    bool hasContext() const noexcept { return static_cast<bool>(context); }

    int totalExpected() const noexcept {
        return context ? context->totalExpected : 0;
    }
    int totalActual() const noexcept {
        return context ? context->totalActual : 0;
    }
    int missingQuantity() const noexcept {
        return context ? context->missingQuantity() : 0;
    }

    QString groupKey() const {
        return context ? context->group.groupKey() : QString();
    }

    int groupSize() const noexcept {
        return context ? context->group.size() : 0;
    }

    bool isGrouped() const noexcept {
        return context && context->group.size() > 1;
    }

    AuditContext* contextPtr() const noexcept { return context.get(); }

    // Tároló UUID lekérése a StockRegistry-ből
    QUuid storageId() const {
        std::optional<StockEntry> s =
            StockRegistry::instance().findById(stockEntryId);
        if(!s.has_value()) return QUuid();
        return s.value().storageId;
    }



    AuditStatus statusType() const {
        // 🔹 Hulló sor – mindig saját státusz alapján
        if (sourceType == AuditSourceType::Leftover) {
            return status();
        }

        // 🔹 Egyedi sor – nincs csoport vagy csak 1 elem
        if (!context || context->group.size() <= 1) {
            return status();
        }

        // 🔹 Csoportos sor
        if (isAudited()) {
            // saját jogon auditált → mutassa a saját státuszt (zöld/piros)
            return status();
        } else {
            // nem auditált, de a csoport részlegesen auditált → legyen narancs
            if (context->isGroupPartiallyAudited()) {
                return AuditStatus(AuditStatus::Audited_Partial);
            }
            // nem auditált, de a csoport részben teljesült → legyen sárga
            if (context->totalActual > 0 && context->totalActual < context->totalExpected) {
                return AuditStatus(AuditStatus::Audited_Unfulfilled);
            }
            // különben marad nem auditált
            return AuditStatus(AuditStatus::NotAudited);
        }
    }

    AuditStatus status() const {
        // 🔹 Hulló sorok
        if (sourceType == AuditSourceType::Leftover) {
            if (!isAudited()) {
                if (isInOptimization && totalExpected() > 0) {
                    // Van elvárás, de még nem auditálták
                    return AuditStatus(AuditStatus::NotAudited);
                }
                // Tényleg nincs elvárás → csak regisztrált
                return AuditStatus(AuditStatus::RegisteredOnly);
            }
            // Auditált leftover → fulfilled vagy missing
            return isFulfilled()
                       ? AuditStatus(AuditStatus::Audited_Fulfilled)
                       : AuditStatus(AuditStatus::Audited_Missing);
        }

        // 🔹 Stock sorok
        if (!isAudited()) {
            if (totalExpected() > 0) {
                // Van elvárás, de még nem auditálták
                return AuditStatus(AuditStatus::NotAudited);
            }
            // Nincs elvárás → regisztrált
            return AuditStatus(AuditStatus::RegisteredOnly);
        }

        // Auditált stock sor → fulfilled vagy missing
        return isFulfilled()
                   ? AuditStatus(AuditStatus::Audited_Fulfilled)
                   : AuditStatus(AuditStatus::Audited_Missing);
    }

    QString suffixForRow() const {
        if (sourceType == AuditSourceType::Leftover) {
            return isAudited()
            ? (isFulfilled()
                   ? AuditStatus::suffix_HulloVan()
                   : AuditStatus::suffix_HulloNincs())
            : AuditStatus::suffix_HulloNemAudit();
        }

        if (!context || context->group.size() <= 1) {
            if (isRowModified)
                return isFulfilled()
                           ? AuditStatus::suffix_Modositva()
                           : AuditStatus::suffix_ModositvaNincs();
            if (isRowAuditChecked && !isFulfilled())
                return AuditStatus::suffix_NincsKeszlet();
        }

        return QString(); // nincs suffix
    }

    QString statusText() const {
        const AuditStatus s = statusType();
        const QString suffix = suffixForRow();
        return suffix.isEmpty()
                   ? s.toDecoratedText()
                   : AuditStatus::withSuffix(s.get(), suffix);
    }


};

