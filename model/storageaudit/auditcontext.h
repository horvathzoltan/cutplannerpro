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

    //int confirmedCount = 0;   // ✅ hány sor van pipálva
    //int totalCount = 0;       // 📋 hány sor tartozik a csoportba

    [[nodiscard]] int totalCount() const noexcept {
        return group.size();
    }

    int confirmedCount = 0;   // kívülről vezetve:  ✅ hány sor van pipálva;
   // [[nodiscard]] int confirmedCount() const;
    /**
 * @brief Csoportszintű hiány számítása.
 *
 * Az AuditContext azonos anyag+hely csoport összesített adatait tartalmazza.
 * Ez a metódus a teljes csoport hiányát adja vissza:
 *   totalExpected - totalActual
 *
 * @note Az eredmény mindig 0 és totalExpected közé van szorítva.
 *       - Negatív hiány nem fordulhat elő.
 *       - A hiány nem lehet nagyobb, mint az elvárt mennyiség.
 *
 * @return int A csoport szintű hiányzó mennyiség.
 */
     [[nodiscard]] int missingQuantity() const noexcept {
         return std::clamp(totalExpected - totalActual, 0, totalExpected);
     }

     [[nodiscard]] AuditPresence groupPresence() const {
         int confirmed = confirmedCount;
         if (confirmed == 0) return AuditPresence::Unknown;
         if (confirmed < totalCount()) return AuditPresence::Missing;
         return AuditPresence::Present;
     }

     [[nodiscard]] AuditResult groupAuditResult() const {
         int confirmed = confirmedCount;
         if (confirmed == 0) return AuditResult::NotAudited;
         if (confirmed < totalCount()) return AuditResult::AuditedPartial;
         return AuditResult::AuditedOk;
     }

};
