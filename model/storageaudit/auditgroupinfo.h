#pragma once

#include <QString>
#include <QList>
#include <QUuid>

/**
 * @brief Egy audit csoporthoz tartozó metaadatok
 *        (anyag + tároló + sorok + összesített értékek)
 */
struct AuditGroupInfo {
    QUuid materialId;           // 📦 Anyag azonosító
    QString groupKey;           // 🔑 Csoport kulcs (pl. storageName vagy auditGroup)
    QList<QUuid> rowIds;        // 📋 Audit sorok azonosítói ebben a csoportban

    int totalExpected = 0;      // 🎯 Összes elvárt mennyiség a csoportban
    int totalActual = 0;        // ✅ Összes tényleges mennyiség a csoportban

    // 🧠 Bővíthető mezők későbbre:
    // QString requiredAtMachine;
    // QString visualLabel;
    // bool isCritical;
};
