#pragma once

#include <QString>


/**
 * @brief Vágási terv státusza — a teljesülés vagy elakadás lekövetésére
 */

namespace Cutting{
namespace Plan{

enum class Status {
    NotStarted,   // 🔹 Még nincs vágás
    InProgress,   // ✂️ Már történt vágás
    Completed,    // ✅ Teljesen befejezett terv
    Abandoned     // ❌ Félbemaradt, kézzel lezárt terv
};

namespace StatusUtils{
    inline QString toDisplayText(Status s) {
        switch (s) {
        case Status::NotStarted:   return "Not started";
        case Status::InProgress:   return "In progress";
        case Status::Completed:    return "Completed";
        default:                   return "Unknown";
        }
    }

    inline QString toCsv(Status s) {
        switch (s) {
        case Status::NotStarted: return "NOT_STARTED";
        case Status::InProgress: return "IN_PROGRESS";
        case Status::Completed:  return "COMPLETED";
        case Status::Abandoned:  return "ABANDONED";
        }
        return "NOT_STARTED";
    }

    inline Status fromCsv(const QString& s) {
        if (s.compare("NOT_STARTED", Qt::CaseInsensitive) == 0) return Status::NotStarted;
        if (s.compare("IN_PROGRESS", Qt::CaseInsensitive) == 0) return Status::InProgress;
        if (s.compare("COMPLETED", Qt::CaseInsensitive) == 0)  return Status::Completed;
        if (s.compare("ABANDONED", Qt::CaseInsensitive) == 0)  return Status::Abandoned;
        return Status::NotStarted;
    }

} // end namespace StatusUtils
} // namespace Plan
} // namespace Cutting

