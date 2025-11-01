#pragma once

#include "model/storageaudit/storageauditrow.h"
#include "model/storageaudit/auditstatus.h"

namespace AuditStatusText {

/**
 * @brief Enum → emberi olvasható szöveg
 */
QString toText(const AuditStatus& s);

/**
 * @brief Sorhoz tartozó suffix meghatározása (pl. "Hulló van", "Módosítva").
 */
QString suffixForRow(const StorageAuditRow& row);

/**
 * @brief Teljes szöveg előállítása (alap státusz + suffix, ha van).
 */
QString decorated(const StorageAuditRow& row);

} // namespace AuditStatusText
