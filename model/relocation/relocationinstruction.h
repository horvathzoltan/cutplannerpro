#pragma once

#include "model/storageaudit/storageauditrow.h"
#include <QString>

struct RelocationSourceEntry {
    QUuid locationId;       ///< Forrás tárhely azonosító
    QString locationName;   ///< Forrás tárhely neve
    int available = 0;      ///< Elérhető mennyiség a stockban
    int moved = 0;          ///< Innen ténylegesen elmozgatott mennyiség
};

struct RelocationTargetEntry {
    QUuid locationId;       ///< Cél tárhely azonosító
    QString locationName;   ///< Cél tárhely neve
    int placed = 0;         ///< Ide ténylegesen lerakott mennyiség
};

#pragma once

#include "model/storageaudit/storageauditrow.h"
#include <QString>
#include <QUuid>
#include <optional>
#include <QVector>

/**
 * @brief Egy relokációs terv egyetlen sorát reprezentáló adatstruktúra.
 *
 * A RelocationInstruction tartalmaz minden információt, ami szükséges ahhoz,
 * hogy egy anyag mozgatását vagy meglétének megerősítését a felhasználó
 * számára egyértelműen megjelenítsük a táblában.
 *
 * Mezők:
 * - materialName: az anyag kódja vagy megnevezése
 * - plannedQuantity: a terv szerinti mozgatandó mennyiség
 * - executedQuantity: a ténylegesen végrehajtott mennyiség (Finalize után rögzül)
 * - sources: forrás tárhelyek és mennyiségek listája
 * - targets: cél tárhelyek és mennyiségek listája
 * - isSatisfied: jelzi, hogy a terv teljesült-e (✔ Megvan státusz)
 * - barcode: azonosító, különösen hullóknál fontos (egyedi rúd)
 * - sourceType: a forrás típusa (Stock vagy Hulló)
 * - materialId: az anyag egyedi azonosítója (UUID)
 */
struct RelocationInstruction {

    // Normál sor ctor
    explicit RelocationInstruction(const QString& materialName,
                                   int plannedQuantity,
                                   bool isSatisfied,
                                   const QString& barcode,
                                   AuditSourceType sourceType,
                                   const QUuid& materialId)
        : materialName(materialName),
        plannedQuantity(plannedQuantity),
        isSatisfied(isSatisfied),
        barcode(barcode),
        sourceType(sourceType),
        materialId(materialId)
    {}

    // Összesítő sor ctor
    RelocationInstruction(const QString& materialName,
                          int requiredQty,
                          int totalRemaining,
                          int auditedRemaining,
                          int movedQty,
                          int uncoveredQty,
                          int coveredQty,          // 🔹 tényleges lefedettség
                          int usedFromRemaining,   // 🔹 ténylegesen felhasznált maradék
                          const QString& statusText,
                          const QString& barcode,
                          AuditSourceType sourceType,
                          const QUuid& materialId)
        : materialName(materialName),
        plannedQuantity(requiredQty),
        executedQuantity(movedQty),
        isSatisfied(uncoveredQty == 0),
        barcode(barcode),
        sourceType(sourceType),
        materialId(materialId),
        isSummary(true),
        summaryText(statusText),
        totalRemaining(totalRemaining),
        auditedRemaining(auditedRemaining),
        movedQty(movedQty),
        uncoveredQty(uncoveredQty),
        coveredQty(coveredQty),
        usedFromRemaining(usedFromRemaining)   // 🔹 kitöltjük
    {}

    QString materialName;
    int plannedQuantity;
    std::optional<int> executedQuantity;

    QVector<RelocationSourceEntry> sources;
    QVector<RelocationTargetEntry> targets;

    bool isSatisfied = false;
    QString barcode;
    AuditSourceType sourceType;
    QUuid materialId;

    // Összesítő sor mezők
    bool isSummary = false;
    QString summaryText;
    int totalRemaining = 0;       ///< Teljes készlet (auditált + nem auditált)
    int auditedRemaining = 0;     ///< Auditált készlet
    int movedQty = 0;             ///< Odavitt mennyiség
    int uncoveredQty = 0;         ///< Lefedetlen igény
    int coveredQty = 0;           ///< Igényből ténylegesen lefedett mennyiség
    int usedFromRemaining = 0;    ///< 🔹 Lefedéshez ténylegesen felhasznált maradék
};
