// // AuditContext.cpp
// #include "auditcontext.h"
// #include "storageauditrow.h"

// int AuditContext::confirmedCount() const {
//     if (!group.isGroup() && group.isEmpty()) {
//         return 0;
//     }

//     int count = 0;
//     // végigmegyünk a csoporthoz tartozó rowId-kon
//     for (const QUuid& memberId : group.rowIds()) {
//         // minden StorageAuditRow-t meg kell keresni a globális listában
//         // (pl. egy registry-ben vagy egy központi rows listában)
//         // itt feltételezzük, hogy van egy globális elérési pont:
//         extern const QList<StorageAuditRow>& g_allAuditRows;

//         auto it = std::find_if(g_allAuditRows.begin(), g_allAuditRows.end(),
//                                [&](const StorageAuditRow& r){ return r.rowId == memberId; });
//         if (it != g_allAuditRows.end() && it->isRowAuditChecked) {
//             ++count;
//         }
//     }
//     return count;
// }


