#pragma once

#include <QString>
#include <QColor>

namespace Relocation {

enum class AuditStatus {
    Uncovered,     // van lefedetlen igény
    NotAudited,    // van maradék, de nem auditált
    Covered        // minden auditált, igény teljesítve
};

class AuditStatusHelper {
public:
    static AuditStatus fromInstruction(int uncoveredQty,
                                       int auditedRemaining,
                                       int totalRemaining)
    {
        if (uncoveredQty > 0) {
            return AuditStatus::Uncovered;   // piros
        }
        if (totalRemaining == 0) {
            return AuditStatus::Covered;     // nincs maradék → teljesen auditált
        }
        if (auditedRemaining >= totalRemaining) {
            return AuditStatus::Covered;     // minden maradék auditált
        }
        return AuditStatus::NotAudited;      // maradt auditálatlan → sárga
    }


    static QColor color(AuditStatus status) {
        switch (status) {
        case AuditStatus::Uncovered:   return QColor("#B22222"); // 🔴 piros
        case AuditStatus::NotAudited:  return QColor("#FFD700"); // 🟡 sárga
        case AuditStatus::Covered:     return QColor("#228B22"); // 🟢 zöld
        }
        return QColor("#999999"); // fallback
    }

    static QString icon(AuditStatus status) {
        switch (status) {
        case AuditStatus::Uncovered:   return "🔴";
        case AuditStatus::NotAudited:  return "🟡";
        case AuditStatus::Covered:     return "🟢";
        }
        return "⚪";
    }

    static QString text(AuditStatus status) {
        switch (status) {
        case AuditStatus::Uncovered:   return "Auditálatlan hiány";
        case AuditStatus::NotAudited:  return "Részlegesen auditált";
        case AuditStatus::Covered:     return "Teljesen auditált";
        }
        return "Ismeretlen státusz";
    }
};

} // namespace Relocation
