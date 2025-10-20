#pragma once

#include <QList>
#include <QUuid>
#include "auditgroupinfo.h"
#include "model/storageaudit/audit_enums.h"

struct StorageAuditRow; // forward declaration

struct AuditContext {
public:
    //AuditContext() = default;   // ✅ engedélyezi a default konstruktort
    explicit AuditContext(const QString& groupKey, const QUuid& matId)
        : materialId(matId), group(groupKey) {}

    QUuid materialId;           // 📦 Anyag azonosító
    int totalExpected = 0;      // 🎯 Összes elvárt mennyiség
    int totalActual = 0;        // ✅ Összes tényleges mennyiség
    AuditGroupInfo group;       // Csoport metaadat

    QList<StorageAuditRow*> _groupRows; // 🔗 közvetlen pointerek a sorokra

    [[nodiscard]] int totalCount() const noexcept {
        return _groupRows.size();
    }

    void setGroupRows(const QList<StorageAuditRow*>& rows) {
        _groupRows = rows;
    }

    int confirmedCount() const;

    [[nodiscard]] int missingQuantity() const noexcept {
         return std::clamp(totalExpected - totalActual, 0, totalExpected);
     }

     [[nodiscard]] AuditPresence groupPresence() const {
         int confirmed = confirmedCount();
         if (confirmed == 0) return AuditPresence::Unknown;
         if (confirmed < totalCount()) return AuditPresence::Missing;
         return AuditPresence::Present;
     }

     // [[nodiscard]] AuditResult groupAuditResult() const {
     //     int confirmed = confirmedCount();
     //     if (confirmed == 0) return AuditResult::NotAudited;
     //     if (confirmed < totalCount()) return AuditResult::AuditedPartial;
     //     return AuditResult::AuditedOk;
     // }

};
