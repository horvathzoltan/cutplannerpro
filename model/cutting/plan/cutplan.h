#pragma once  // 👑 Modern include guard

#include "model/cutting/cuttingmachine.h"
#include "model/cutting/plan/parentinfo.h"
#include "status.h"
#include "source.h"
#include "segments.h"
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
    QUuid planId = QUuid::createUuid(); // ✅ automatikus UUID, egyedi tervazonosító
    int planNumber = -1;   // 🔢 Globális batch-sorszám (planCounter)

    Status status = Status::NotStarted; // tervállapot
    QString rodId;            // 🔑 Stabil rúd azonosító (ROD-xxxx)
    // 🔑 Stabil, emberi szemnek szánt rúd-azonosító (Rod-A, Rod-CA, ...).
    // Minden CutPlan kap egy új rodId-t, ami auditban és UI-ban a "RodRef".

    // 🔗 Anyag törzs azonosító (UUID a MaterialMaster-ből).
    // Ez alapján jön a materialName(), materialGroupName(), materialBarcode().
    // Nem egyedi a konkrét rúdra, csak az anyagtípusra mutat.
    QUuid materialId;         // 🔗 Az anyag azonosítója (UUID)

    Cutting::Plan::Source source = Cutting::Plan::Source::Stock; // anyag forrásas
    // 🆕 Fizikai forrás azonosítója a konkrét rúdnál:
    // - Stock esetén: mesterségesen generált MAT-001, MAT-002, ...
    // - Leftover esetén: a leftover saját RST-xxx kódja.
    // Ez az érték mindig egyedi a konkrét rúdra, és auditban a "Barcode" mező.
    QString sourceBarcode;   // 🆕 Mindig kitöltött: MAT-xxx vagy RST-xxx

    QUuid machineId;   // a gép azonosítója
    QString machineName;   // csak UI/audit
    double machineKerf = 0.0; // audit fixálás




    int optimizationId; // 🔢 Az optimalizációs futás azonosítója (kötelező)



    Segments _segments;

    // bemenő adatok- ezeket "keressük"
    QVector<Cutting::Piece::PieceWithMaterial> piecesWithMaterial; // ✂️ Levágott darabok — anyaggal együtt

    Cutting::Piece::PieceWithMaterial getPieceMaterialBy_pieceId(const QUuid& id) const;
    Cutting::Piece::PieceInfo getPieceInfoBy_pieceId(const QUuid& id) const;

    // 🧠 Viselkedésalapú metódusok
    bool isReusable() const;
    bool isStockRod() const;
    bool isFinalized() const;

    QString materialName() const;        // Anyag neve — materialId alapján
    QString materialGroupName() const;   // Anyag csoportneve — helper alapján

    Status getStatus() const;
    void setStatus(Status newStatus);


    /**
     * @brief 📐 Szakaszgenerálás helper - vágási szakaszok generálása a darabok és paraméterek alapján
     * @param kerf_mm Vágási veszteség mm-ben
     * @param totalLength_mm Az anyag teljes hossza mm-ben
     *
     * A szakaszok a piecesWithMaterial alapján jönnek létre.
     * A SegmentUtils::generateSegments metódust használja.
     */

    // void generateSegments(double kerf_mm, int totalLength_mm){
    //     auto seg1 =
    //         Cutting::Segment::SegmentUtils::generateSegments(
    //             this->piecesWithMaterial,
    //             kerf_mm, totalLength_mm);

    //     _segments.setSegments(seg1);
    //     _segments.SetTotalLength_mm(totalLength_mm);

    //     // ♻️ leftover barcode generálás
    //     int w = _segments.waste_mm();
    //     if (w >= 300) {
    //         int id = SettingsManager::instance().nextLeftoverCounter();
    //         auto barcode = IdentifierUtils::makeLeftoverId(id);
    //         _segments.setLeftoverBarcode(barcode);
    //     }
    // }

    // a material törzsből jön (MAT-ROLL60-6000), mivel a törzsből jövő kódot sokszor szeretnénk látni (pl. anyagazonosítás, csoportosítás, riport).
    // és emiatt sok helyen félrevezető, mert a GUI tooltipben úgy tűnik, mintha ez lenne a sourceBarcode.
    QString materialBarcode() const;

    QString toLogEntry(const CuttingMachine& machine) const;
private:
    std::optional<ParentInfo> _parent = std::nullopt;

public:
    void setParent(const ParentInfo& parent) {
        zInfo("⚠ CutPlan::setParent WRITE: " + parent.barcode());
        _parent = parent;
    }

    void clearParent() {
        zInfo("⚠ CutPlan::clearParent");
        _parent.reset();
    }

    const std::optional<ParentInfo>& parent() const {
        return _parent;
    }

};
}  //endof namespace Plan
}  //endof namespace Cutting

