#pragma once

#include "model/storageaudit/storageauditrow.h"
#include <QString>

/**
 * @brief Egy relokációs terv egyetlen sorát reprezentáló adatstruktúra.
 *
 * A RelocationInstruction tartalmaz minden információt, ami szükséges ahhoz,
 * hogy egy anyag mozgatását vagy meglétének megerősítését a felhasználó
 * számára egyértelműen megjelenítsük a táblában.
 *
 * Mezők:
 * - materialCode: az anyag kódja vagy megnevezése
 * - sourceLocation: a forrás tároló (honnan kell mozgatni)
 * - targetLocation: a cél tároló (hova kell mozgatni)
 * - quantity: a mozgatandó mennyiség (ha 0, akkor nincs tényleges mozgatás)
 * - isSatisfied: jelzi, hogy a terv teljesült-e (✔ Megvan státusz)
 * - barcode: azonosító, különösen hullóknál fontos (egyedi rúd)
 * - sourceType: a forrás típusa (Stock vagy Hulló)
 */
struct RelocationInstruction {

    explicit RelocationInstruction(const QString& materialName,
                                   const QString& targetLocation,
                                   const QString& sourceLocation,
                                   int plannedQuantity,
                                   bool isSatisfied,
                                   const QString& barcode,
                                   AuditSourceType sourceType,
                                   const QUuid& materialId)
        : materialName(materialName),
        sourceLocation(sourceLocation),
        targetLocation(targetLocation),
        plannedQuantity(plannedQuantity),
        isSatisfied(isSatisfied),
        barcode(barcode),
        sourceType(sourceType),
        materialId(materialId)
    {}

    //QUuid rowId = QUuid::createUuid(); // Egyedi azonosító (CRUD műveletekhez is kell)
    QString materialName;      ///< Anyag kódja vagy megnevezése
    QString sourceLocation;    ///< Forrás tároló neve
    QString targetLocation;    ///< Cél tároló neve
    int plannedQuantity;              ///< Mozgatandó mennyiség
    bool isSatisfied = false;  ///< Ha true → ✔ Megvan, nincs tényleges mozgatás
    QString barcode;           ///< Egyedi azonosító (különösen hullóknál fontos)
    AuditSourceType sourceType;///< Forrás típusa (Stock / Hulló)
    QUuid materialId;    ///< Anyag azonosító (UUID)
};
