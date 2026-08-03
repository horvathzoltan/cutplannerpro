#pragma once

#include <QString>
#include <QHash>
#include <QVariant>
#include <QUuid>

#include <model/cutting/plan/request.h>

#include <model/cutting/piece/piecewithmaterial.h>

#include <materials/model/material_master.h>

#include "product/model/material_role.h"

class KittingInstruction
{
public:
    // Globális lépés ID (mint a CutInstruction::globalStepId)
    int globalStepId = 0;

    // Melyik requesthez tartozik
    QUuid requestId;

    // A termék anyagszerepköre (pl. Motor, Dugó, Pofa, stb.)
    MaterialRole role;

    // Anyag azonosító
    QUuid materialId;

    // Külső referencia (pl. megrendelés száma)
    QString externalReference;

    // Paraméterek (szín, hossz, extra attribútumok)
    QMap<QString, QString> attributes;

    // Fallback / alternatív anyag használata
    bool fallbackUsed = false;
    bool alternativeUsed = false;

    // A szerepkörön belüli sorszám (mint a pieceCounter)
    int roleCounter = 0;

    double quantity = 1.0;        // darab vagy méter
    QString unit = "db";          // vagy "mm", "m"

    // Későbbi státuszokhoz (Pending, Completed, Missing)
    enum class Status {
        Pending,
        Completed,
        Missing
    };

    Status status = Status::Pending;

    KittingInstruction() = default;

    static KittingInstruction makeKitItem(
        const Cutting::Plan::Request& req,
        const Cutting::Piece::PieceWithMaterial& pwm,
        const MaterialMaster* mat,
        const QString& logicalRole,
        double quantity = 1.0,
        const QString& unit = "db")
    {
        KittingInstruction ki;
        ki.requestId    = req.requestId;
        ki.materialId   = mat ? mat->id : QUuid();
        ki.externalReference = req.externalReference;
        ki.attributes   = req.attributes;   // vagy pwm.attributes, ha releváns
        ki.fallbackUsed    = false;
        ki.alternativeUsed = false;

        // szerepkör: a termék szerepköre + logicalRole
        ki.role.productTypeId    = req.productTypeId;
        ki.role.productSubtypeId = req.productSubtypeId;
        ki.role.family           = mat ? mat->family : MaterialFamily::Unknown;
        ki.role.barcodePrefix    = logicalRole;   // pl. "NP-MOT", "NP-POFA", stb.

        ki.quantity = quantity;
        ki.unit     = unit;
        return ki;
    }
};
