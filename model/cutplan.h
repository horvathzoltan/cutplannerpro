#pragma once  // 👑 Modern include guard

#include "common/segmentutils.h"
#include "model/cutting/piecewithmaterial.h"
#include "pieceinfo.h"
#include "segment.h"

#include <QString>
#include <QVector>
#include <QUuid>

/**
 * @brief Vágási terv státusza — a teljesülés vagy elakadás lekövetésére
 */
enum class CutPlanStatus {
    NotStarted,   // 🔹 Még nincs vágás
    InProgress,   // ✂️ Már történt vágás
    Completed,    // ✅ Teljesen befejezett terv
    Abandoned     // ❌ Félbemaradt, kézzel lezárt terv
};

enum class CutPlanSource {
    Stock,     // 🧱 Normál profilkészlet
    Reusable   // ♻️ Hulladékból újravágás
};


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
    QString rodId;                   // 📄 Reusable barcode, ha van

    CutPlanSource source = CutPlanSource::Stock;

    // 🔁 Állapotkezelés
    CutPlanStatus status = CutPlanStatus::NotStarted;

    QUuid planId = QUuid::createUuid(); // ✅ automatikus UUID, egyedi tervazonosító

    QVector<Segment> segments; // 🧱 Vágási szakaszlista

    //QVector<PieceInfo> piecesInfo;

    QVector<PieceWithMaterial> cuts;

    // 🧠 Viselkedésalapú metódusok
    bool usedReusable() const;
    bool isFinalized() const;

    QString name() const;        // Anyag neve — materialId alapján
    QString groupName() const;   // Anyag csoportneve — helper alapján

    CutPlanStatus getStatus() const;
    void setStatus(CutPlanStatus newStatus);

    QString pieceLengthsAsString() const;

    // 📐 Szakaszgenerálás helper
    void generateSegments(int kerf_mm, int totalLength_mm){
        this->segments = SegmentUtils::generateSegments(this->cuts
                                                        /* PieceWithMaterial-ek */,
                                                        kerf_mm, totalLength_mm);

    }
};
