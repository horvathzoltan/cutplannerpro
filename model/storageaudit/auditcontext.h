#pragma once

#include <QList>
#include <QUuid>
#include "auditgroupinfo.h"

struct AuditContext {
public:
    //AuditContext() = default;   // ✅ engedélyezi a default konstruktort
    explicit AuditContext(const QString& groupKey, const QUuid& matId)
        : materialId(matId), group(groupKey) {}

    QUuid materialId;           // 📦 Anyag azonosító
    int totalExpected = 0;      // 🎯 Összes elvárt mennyiség
    int totalActual = 0;        // ✅ Összes tényleges mennyiség

     AuditGroupInfo group; // Csoport metaadat
};
