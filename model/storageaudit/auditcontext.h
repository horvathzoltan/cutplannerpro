#pragma once

#include <QList>
#include <QUuid>
#include "auditgroupinfo.h"
#include "auditstatus.h"
//#include "model/storageaudit/audit_enums.h"

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

    bool isGrouped() const noexcept {
        return group.size() > 1;
    }

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

    const QList<StorageAuditRow*>& groupRows() const noexcept {
        return _groupRows;
    }
    /**
 * @brief Megállapítja, hogy a csoport legalább egy sora auditált-e.
 *
 * A csoport auditáltnak számít, ha legalább egy sorban történt felhasználói beavatkozás:
 * - módosították a mennyiséget (`isRowModified == true`), vagy
 * - bepipálták auditáltnak (`isRowAuditChecked == true`).
 *
 * Ez a függvény nem vizsgálja, hogy a mennyiség teljesült-e, csak azt, hogy történt-e audit.
 * A `statusType()` és `statusText()` függvények ezt használják annak eldöntésére,
 * hogy a csoport auditáltnak tekinthető-e.
 *
 * @return true, ha legalább egy sor auditált; különben false.
 */
    [[nodiscard]] bool isGroupAudited() const;

    [[nodiscard]] bool isGroupPartiallyAudited() const;

     /**
 * @brief Megállapítja, hogy a csoport összes sora teljesítette-e az elvárt mennyiséget.
 *
 * A csoport akkor tekinthető teljesültnek, ha minden sorban:
 * `actualQuantity >= pickingQuantity` → az igény teljesült.
 *
 * Ez a függvény nem vizsgálja, hogy auditált-e a sor, csak a mennyiségi teljesülést.
 * A `AuditStatus::fromGroup()` és `statusType()` ezt használják annak eldöntésére,
 * hogy a csoport státusza "OK" vagy "Pending" legyen.
 *
 * @return true, ha minden sor teljesült; különben false.
 */
    [[nodiscard]] bool isGroupFulfilled() const;

    AuditStatus status() const {
        if (totalCount() == 0) return AuditStatus(AuditStatus::NotAudited);
        if (!isGroupAudited()) return AuditStatus(AuditStatus::NotAudited);
        if (isGroupPartiallyAudited()) return AuditStatus(AuditStatus::Audited_Partial);
        return isGroupFulfilled()
                   ? AuditStatus(AuditStatus::Audited_Fulfilled)
                   : AuditStatus(AuditStatus::Audited_Unfulfilled);
    }

    QString statusText() const {
        AuditStatus s = status();
        const QString base = AuditStatus::statusEmoji(s.get()) + " " + s.toText();

        switch (s.get()) {
        case AuditStatus::Audited_Partial:
        case AuditStatus::Audited_Unfulfilled:
        case AuditStatus::Audited_Fulfilled:
            return base + " (csoport)";
        default:
            return base;
        }
    }




};
