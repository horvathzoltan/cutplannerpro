#pragma once

#include "model/relocation/relocationsourceentry.h"
#include "model/relocation/relocationtargetentry.h"
#include "model/storageaudit/storageauditrow.h"
#include <QString>
#include <QUuid>
#include <optional>
#include <QVector>

/**
 * @brief Egy relokációs terv egyetlen sorát reprezentáló adatstruktúra.
 *
 * A RelocationInstruction tartalmaz minden információt, ami szükséges ahhoz,
 * hogy egy anyag mozgatását, meglétét vagy lefedettségét a felhasználó
 * számára egyértelműen megjelenítsük a relokációs táblában.
 *
 * Mezők:
 * - materialName: az anyag kódja vagy megnevezése
 * - plannedQuantity: a terv szerinti mozgatandó mennyiség (igény)
 * - executedQuantity: a ténylegesen végrehajtott mennyiség (Finalize után rögzül)
 * - isFinalized: jelzi, hogy a sor véglegesítve lett-e (többször nem módosítható)
 * - sources: forrás tárhelyek és az onnan elmozgatott mennyiségek listája
 * - targets: cél tárhelyek és az oda lerakott mennyiségek listája
 * - isSatisfied: jelzi, hogy a terv teljesült-e (✔ Megvan státusz)
 * - barcode: azonosító, különösen hullóknál fontos (egyedi rúd)
 * - sourceType: a forrás típusa (Stock vagy Hulló)
 * - materialId: az anyag egyedi azonosítója (UUID)
 *
 * Összesítő sor esetén:
 * - isSummary: jelzi, hogy a sor összesítő típusú (nem konkrét anyagmozgatás)
 * - summaryText: státusz szöveg (tooltiphez)
 * - totalRemaining: teljes készlet (auditált + nem auditált)
 * - auditedRemaining: auditált készlet
 * - movedQty: odavitt mennyiség (új mozgatás)
 * - uncoveredQty: lefedetlen igény (hiány)
 * - coveredQty: ténylegesen lefedett mennyiség
 * - usedFromRemaining: lefedéshez ténylegesen felhasznált maradék
 *
 * Segédfüggvények:
 * - isReadyToFinalize(): visszaadja, hogy a sor véglegesíthető-e
 * - isAlreadyFinalized(): visszaadja, hogy a sor már véglegesítve lett-e
 * - isFullyCovered(): visszaadja, hogy a dialógusban megadott mennyiségek teljesen lefedik-e az igényt
 * - hasPartialExecution(): visszaadja, hogy történt-e részleges mozgatás (munkaközi állapot)
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
                          int coveredQty,
                          int usedFromRemaining,
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
        usedFromRemaining(usedFromRemaining)
    {}

    QUuid rowId = QUuid::createUuid(); ///< Egyedi sorazonosító – a táblában való hivatkozáshoz

    QString materialName;                     ///< Anyag neve
    int plannedQuantity = 0;                  ///< Igény szerinti mennyiség
    std::optional<int> executedQuantity;      ///< Ténylegesen végrehajtott mennyiség (Finalize után)
    bool isFinalized = false;                 ///< Jelzi, hogy a sor véglegesítve lett-e

    QVector<RelocationSourceEntry> sources;   ///< Forrás tárhelyek
    QVector<RelocationTargetEntry> targets;   ///< Cél tárhelyek

    bool isSatisfied = false;                 ///< ✔ Megvan státusz
    QString barcode;                          ///< Vonalkód (különösen hullóknál)
    AuditSourceType sourceType;               ///< Forrás típusa
    QUuid materialId;                         ///< Anyag UUID

    // Összesítő sor mezők
    bool isSummary = false;
    QString summaryText;
    int totalRemaining = 0;       ///< Teljes készlet (auditált + nem auditált)
    int auditedRemaining = 0;     ///< Auditált készlet
    int movedQty = 0;             ///< Odavitt mennyiség
    int uncoveredQty = 0;         ///< Lefedetlen igény
    int coveredQty = 0;           ///< Igényből ténylegesen lefedett mennyiség
    int usedFromRemaining = 0;    ///< 🔹 Lefedéshez ténylegesen felhasznált maradék

    // 🔹 Megadja, hogy a sor véglegesíthető-e (azaz a dialógusban minden mennyiség meg van adva)
    bool isReadyToFinalize() const {
        // Csak akkor finalizálható, ha van executedQuantity, és az pontosan megegyezik a plannedQuantity-vel
        return executedQuantity.has_value() && executedQuantity.value() == plannedQuantity;
    }

    // 🔹 Megadja, hogy a sor már véglegesítve lett-e
    bool isAlreadyFinalized() const {
        return isFinalized;
    }

    // 🔹 Megadja, hogy a dialógusban megadott mennyiségek teljesen lefedik-e az igényt
    bool isFullyCovered() const {
        // Akkor tekintjük teljesen lefedettnek, ha a sources + targets összege pontosan megegyezik a plannedQuantity-vel
        int totalMoved = 0;
        for (const auto& src : sources)
            totalMoved += src.moved;

        int totalPlaced = 0;
        for (const auto& tgt : targets)
            totalPlaced += tgt.placed;

        return totalMoved == plannedQuantity && totalPlaced == plannedQuantity;
    }

    // 🔹 Megadja, hogy a dialógusban van-e bármilyen adat (azaz elkezdett munka)
    bool hasPartialExecution() const {
        int totalMoved = 0;
        for (const auto& src : sources)
            totalMoved += src.moved;

        int totalPlaced = 0;
        for (const auto& tgt : targets)
            totalPlaced += tgt.placed;

        return totalMoved > 0 || totalPlaced > 0;
    }

};
