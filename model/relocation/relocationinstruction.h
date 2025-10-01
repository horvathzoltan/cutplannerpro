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

    QString materialName;      ///< Anyag kódja vagy megnevezése
    int plannedQuantity;       ///< Terv szerinti mozgatandó mennyiség
    std::optional<int> executedQuantity; ///< Végrehajtott mennyiség (Finalize után rögzül)

    QVector<RelocationSourceEntry> sources; ///< Forrás tárhelyek listája
    QVector<RelocationTargetEntry> targets; ///< Cél tárhelyek listája

    bool isSatisfied = false;  ///< Ha true → ✔ Megvan, nincs tényleges mozgatás
    QString barcode;           ///< Egyedi azonosító (különösen hullóknál fontos)
    AuditSourceType sourceType;///< Forrás típusa (Stock / Hulló)
    QUuid materialId;          ///< Anyag azonosító (UUID)
};
