#pragma once

#include <QList>
#include <QUuid>
#include "auditgroupinfo.h"

struct AuditContext {
    QUuid materialId;           // 📦 Anyag azonosító
    int totalExpected = 0;      // 🎯 Összes elvárt mennyiség
    int totalActual = 0;        // ✅ Összes tényleges mennyiség

     AuditGroupInfo group; // Csoport metaadat
};
