#pragma once  // 👑 Modern include guard

#include "status.h"
#include "source.h"
#include "service/cutting/segment/segmentutils.h"
#include "model/cutting/piece/piecewithmaterial.h"
//#include "../piece/pieceinfo.h"
#include "../segment/segmentmodel.h"

#include <QString>
#include <QVector>
#include <QUuid>



namespace Cutting {
namespace Plan {

/**
 * @class CutPlan
 * @brief Egy adott anyagdarab vágási terve (stock vagy reusable)
 * az anyagfelhasználást írja le, ami ez alapján auditálható
 *
 * Főbb elemek:
 * - piecesWithMaterial: levágott darabok metaadatokkal
 * - segments: vizuális és logikai vágási szakaszok
 * - kerfTotal, waste: anyagveszteség és maradék
 * - materialId, rodId: azonosítók az anyaghoz
 *
 * Használat:
 * - Az OptimizerModel::_result_plans gyűjti össze
 * - Auditálás, újrahasznosítás és UI megjelenítés céljából
 */

class CutPlan
{
public:
    // 📦 Mezők – az eredeti struct-nak megfelelően
    int rodNumber = -1;       // ➕ Sorszám / index
    //QVector<int> cuts;      // ✂️ Darabolások mm-ben
    int kerfTotal = 0;        // 🔧 Vágások során vesztett anyag összesen
    int waste = 0;            // ♻️ Maradék mm
    QUuid materialId;         // 🔗 Az anyag azonosítója (UUID)
    int totalLength = 0;      // 📏 Anyag hossz (mm)
    QString rodId;            // 📄 Reusable barcode, ha van
    QUuid machineId;   // a gép azonosítója
    QString machineName;   // csak UI/audit
    double kerfUsed_mm = 0.0; // audit fixálás

    Cutting::Plan::Source source = Cutting::Plan::Source::Stock; // anyag forrásas

    Status status = Status::NotStarted; // tervállapot

    QUuid planId = QUuid::createUuid(); // ✅ automatikus UUID, egyedi tervazonosító

    QVector<Cutting::Segment::SegmentModel> segments; // 🧱 Vágási szakaszok — vizuális és logikai bontás
    QVector<Cutting::Piece::PieceWithMaterial> piecesWithMaterial; // ✂️ Levágott darabok — anyaggal együtt

    // 🧠 Viselkedésalapú metódusok
    bool isReusable() const;
    bool isFinalized() const;

    QString materialName() const;        // Anyag neve — materialId alapján
    QString materialGroupName() const;   // Anyag csoportneve — helper alapján

    Status getStatus() const;
    void setStatus(Status newStatus);

    QString pieceLengthsAsString() const;

    std::optional<QString> parentBarcode;
    std::optional<QUuid> parentPlanId;

    int planNumber = -1;   // 🔢 Globális batch-sorszám (planCounter)

/**
 * @brief 📐 Szakaszgenerálás helper - vágási szakaszok generálása a darabok és paraméterek alapján
 * @param kerf_mm Vágási veszteség mm-ben
 * @param totalLength_mm Az anyag teljes hossza mm-ben
 *
 * A szakaszok a piecesWithMaterial alapján jönnek létre.
 * A SegmentUtils::generateSegments metódust használja.
 */
    void generateSegments(double kerf_mm, int totalLength_mm){
        this->segments = Cutting::Segment::SegmentUtils::generateSegments(this->piecesWithMaterial
                                                                          /* PieceWithMaterial-ek */,
                                                                          kerf_mm, totalLength_mm);

    }
    QString materialBarcode() const;

    QString machineLabel() const { return QString("%1 (kerf=%2 mm)").arg(machineName).arg(kerfUsed_mm); }
};
}  //endof namespace Plan
}  //endof namespace Cutting

