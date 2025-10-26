#include "segmentutils.h"
//#include "model/cutting/piecewithmaterial.h"
#include "qdebug.h"
//#include <numeric>

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

QVector<SegmentModel> SegmentUtils::generateSegments(
    const QVector<Cutting::Piece::PieceWithMaterial>& cuts,
    double kerf_mm, double totalLength_mm)
{
    QVector<SegmentModel> segments;
    double usedLength = 0;

    for (int i = 0; i < cuts.size(); ++i) {
        const Cutting::Piece::PieceWithMaterial& pwm = cuts[i];
        double len = pwm.info.length_mm;

        // ➕ Darab szakasz
        SegmentModel piece(SegmentModel::Type::Piece, static_cast<int>(len));
        segments.append(piece);
        usedLength += len;

        // ➕ Kerf szakasz – az utolsó után is!
        if (kerf_mm > 0) {
            SegmentModel kerf(SegmentModel::Type::Kerf, static_cast<int>(kerf_mm));
            segments.append(kerf);
            usedLength += kerf_mm;
        }
    }

    // 🧺 Végmaradék, ha van
    double waste = totalLength_mm - usedLength;
    if (waste > 0) {
        SegmentModel trailingWaste(SegmentModel::Type::Waste, static_cast<int>(waste));
        segments.append(trailingWaste);
    } else if (waste < 0) {
        qWarning() << "Vágáshossz + kerf túllépi a rudat!";
    }

    return segments;
}

} // endof namespace Segment
} // endof namespace Cutting::Segment
