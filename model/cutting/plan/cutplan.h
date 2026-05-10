#pragma once  // 👑 Modern include guard

#include "model/cutting/cuttingmachine.h"
#include "status.h"
#include "source.h"
#include "../../../service/cutting/segment/segmentutils.h"
#include "../piece/piecewithmaterial.h"
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
    int kerfTotal = 0;        // 🔧 Vágások során vesztett anyag összesen
    int waste = 0;            // ♻️ Maradék mm

    QUuid materialId;         // 🔗 Az anyag azonosítója (UUID)
    // 🔗 Anyag törzs azonosító (UUID a MaterialMaster-ből).
    // Ez alapján jön a materialName(), materialGroupName(), materialBarcode().
    // Nem egyedi a konkrét rúdra, csak az anyagtípusra mutat.

    int totalLength = 0;      // 📏 Anyag hossz (mm)

    QString rodId;            // 🔑 Stabil rúd azonosító (ROD-xxxx)
    // 🔑 Stabil, emberi szemnek szánt rúd-azonosító (Rod-A, Rod-CA, ...).
    // Minden CutPlan kap egy új rodId-t, ami auditban és UI-ban a "RodRef".

    QUuid machineId;   // a gép azonosítója
    QString machineName;   // csak UI/audit
    double kerfUsed_mm = 0.0; // audit fixálás

    Cutting::Plan::Source source = Cutting::Plan::Source::Stock; // anyag forrásas

    Status status = Status::NotStarted; // tervállapot

    QUuid planId = QUuid::createUuid(); // ✅ automatikus UUID, egyedi tervazonosító

    QVector<Cutting::Segment::SegmentModel> segments; // 🧱 Vágási szakaszok — vizuális és logikai bontás

    // bemenő adatok- ezeket "keressük"
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

    QString leftoverBarcode; // ♻️ Ha a rúd végén leftover keletkezik, itt tároljuk az új barcode-ot
    // ♻️ Ha a vágás után keletkezik új hulló, itt tároljuk az új RST-xxx kódot.
    // Ez NEM a forrás, hanem a kimeneti leftover azonosító.

    int planNumber = -1;   // 🔢 Globális batch-sorszám (planCounter)

    QString sourceBarcode;   // 🆕 Mindig kitöltött: MAT-xxx vagy RST-xxx
    // 🆕 Fizikai forrás azonosítója a konkrét rúdnál:
    // - Stock esetén: mesterségesen generált MAT-001, MAT-002, ...
    // - Leftover esetén: a leftover saját RST-xxx kódja.
    // Ez az érték mindig egyedi a konkrét rúdra, és auditban a "Barcode" mező.

    int optimizationId; // 🔢 Az optimalizációs futás azonosítója (kötelező)

    /**
 * @brief 📐 Szakaszgenerálás helper - vágási szakaszok generálása a darabok és paraméterek alapján
 * @param kerf_mm Vágási veszteség mm-ben
 * @param totalLength_mm Az anyag teljes hossza mm-ben
 *
 * A szakaszok a piecesWithMaterial alapján jönnek létre.
 * A SegmentUtils::generateSegments metódust használja.
 */
    void generateSegments(double kerf_mm, int totalLength_mm){
        this->segments =
            Cutting::Segment::SegmentUtils::generateSegments(
            this->piecesWithMaterial
          /* PieceWithMaterial-ek */,
            kerf_mm, totalLength_mm);
    }


    QString getWasteBarcode(){
        for(auto&a:this->segments){
            if(a.type() == Cutting::Segment::SegmentModel::Type::Waste){
                return a.barcode();
            }
        }
        return {};
    }

    // a material törzsből jön (MAT-ROLL60-6000), mivel a törzsből jövő kódot sokszor szeretnénk látni (pl. anyagazonosítás, csoportosítás, riport).
    // és emiatt sok helyen félrevezető, mert a GUI tooltipben úgy tűnik, mintha ez lenne a sourceBarcode.
    QString materialBarcode() const;

    QString machineLabel() const { return QString("%1 (kerf=%2 mm)").arg(machineName).arg(kerfUsed_mm); }

    QString toLogEntry(const CuttingMachine& machine) const;
};
}  //endof namespace Plan
}  //endof namespace Cutting

