#pragma once

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

}} //endof namespace Cutting::Plan
