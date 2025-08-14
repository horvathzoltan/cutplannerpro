#include "segmentutils.h"
//#include "model/cutting/piecewithmaterial.h"
#include "qdebug.h"
#include <numeric>

// QVector<Segment> SegmentUtils::generateSegments(const QVector<int>& cuts, int kerf_mm, int totalLength_mm)
// {
//     QVector<Segment> segments;

//     int totalCut = 0;
//     int kerfTotal = 0;

//     // 1️⃣ Darab + kerf minden darab után
//     for (int cut : cuts) {
//         segments.append({ cut, SegmentType::Piece });
//         segments.append({ kerf_mm, SegmentType::Kerf });

//         totalCut += cut;
//         kerfTotal += kerf_mm;
//     }

//     // 2️⃣ Az utolsó kerf fizikailag nem kell darabhoz → azonnal hulladék előtt van
//     // Tehát nem távolítjuk el — ez modellhű!

//     // 3️⃣ Végmaradék kiszámítása
//     int usedLength = totalCut + kerfTotal;
//     int waste = totalLength_mm - usedLength;

//     if (waste > 0)
//         segments.append({ waste, SegmentType::Waste });

//     return segments;
// }
namespace Cutting{
namespace Segment{

bool SegmentUtils::isTrailingWaste(int wasteLength, const QVector<SegmentModel>& segments)
{
    if (segments.isEmpty())
        return false;

    const SegmentModel& last = segments.last();
    return (last.type == SegmentModel::Type::Waste && last.length_mm == wasteLength);
}

QVector<SegmentModel> SegmentUtils::generateSegments(const QVector<Cutting::Piece::PieceWithMaterial>& cuts, int kerf_mm, int totalLength_mm)
{
    QVector<SegmentModel> segments;

    int usedLength = 0;

    for (int i = 0; i < cuts.size(); ++i) {
        const Cutting::Piece::PieceWithMaterial& pwm = cuts[i];
        int len = pwm.info.length_mm;

        // ➕ Darab szakasz
        SegmentModel piece;
        piece.length_mm = len;
        piece.type = SegmentModel::Type::Piece;
        segments.append(piece);

        usedLength += len;

    // ➕ Kerf szakasz – az utolsó után is!
        if (kerf_mm > 0){// && i != cuts.size() - 1) {
            SegmentModel kerf;
            kerf.length_mm = kerf_mm;
            kerf.type = SegmentModel::Type::Kerf;
            segments.append(kerf);

            usedLength += kerf_mm;
        }
    }

    // 🧺 Végmaradék, ha van
    int waste = totalLength_mm - usedLength;
    if (waste > 0) {
        SegmentModel trailingWaste;
        trailingWaste.length_mm = waste;
        trailingWaste.type = SegmentModel::Type::Waste;
        segments.append(trailingWaste);
    } else if (waste < 0) {
        qWarning() << "Vágáshossz + kerf túllépi a rudat!";
    }

    return segments;
}
}} // endof namespace Cutting::Segment
