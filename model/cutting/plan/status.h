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

inline QString statusText(Status s) {
    switch (s) {
    case Status::NotStarted:   return "Not started";
    case Status::InProgress:   return "In progress";
    case Status::Completed:    return "Completed";
    default:                   return "Unknown";
    }
}

} // namespace Plan
} // namespace Cutting

