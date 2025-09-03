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
 * @brief Egy konkrét vágási terv — akár reusable, akár szálanyaghoz
 */
class CutPlan
{
public:
    // 📦 Mezők – az eredeti struct-nak megfelelően
    int rodNumber = -1;              // ➕ Sorszám / index
    //QVector<int> cuts;               // ✂️ Darabolások mm-ben
    int kerfTotal = 0;               // 🔧 Vágások során vesztett anyag összesen
    int waste = 0;                   // ♻️ Maradék mm    
    QUuid materialId;                // 🔗 Az anyag azonosítója (UUID)
    int totalLength = 0;      // 📏 Anyag hossz (mm)
    QString rodId;                   // 📄 Reusable barcode, ha van

    Cutting::Plan::Source source = Cutting::Plan::Source::Stock;

    // 🔁 Állapotkezelés
    Status status = Status::NotStarted;

    QUuid planId = QUuid::createUuid(); // ✅ automatikus UUID, egyedi tervazonosító

    QVector<Cutting::Segment::SegmentModel> segments; // 🧱 Vágási szakaszlista

    //QVector<PieceInfo> piecesInfo;

    QVector<Cutting::Piece::PieceWithMaterial> piecesWithMaterial;

    // 🧠 Viselkedésalapú metódusok
    bool isReusable() const;
    bool isFinalized() const;

    QString materialName() const;        // Anyag neve — materialId alapján
    QString materialGroupName() const;   // Anyag csoportneve — helper alapján

    Status getStatus() const;
    void setStatus(Status newStatus);

    QString pieceLengthsAsString() const;

    // 📐 Szakaszgenerálás helper
    void generateSegments(int kerf_mm, int totalLength_mm){
        this->segments = Cutting::Segment::SegmentUtils::generateSegments(this->piecesWithMaterial
                                                        /* PieceWithMaterial-ek */,
                                                        kerf_mm, totalLength_mm);

    }
};
}  //endof namespace Plan
}  //endof namespace Cutting

