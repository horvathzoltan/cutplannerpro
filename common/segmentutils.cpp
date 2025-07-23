#include "segmentutils.h"
#include <numeric>

QVector<Segment> SegmentUtils::generateSegments(const QVector<int>& cuts, int kerf_mm, int totalLength_mm)
{
    QVector<Segment> segments;

    int totalCut = 0;
    int kerfTotal = 0;

    // 1️⃣ Darab + kerf minden darab után
    for (int cut : cuts) {
        segments.append({ cut, SegmentType::Piece });
        segments.append({ kerf_mm, SegmentType::Kerf });

        totalCut += cut;
        kerfTotal += kerf_mm;
    }

    // 2️⃣ Az utolsó kerf fizikailag nem kell darabhoz → azonnal hulladék előtt van
    // Tehát nem távolítjuk el — ez modellhű!

    // 3️⃣ Végmaradék kiszámítása
    int usedLength = totalCut + kerfTotal;
    int waste = totalLength_mm - usedLength;

    if (waste > 0)
        segments.append({ waste, SegmentType::Waste });

    return segments;
}


bool SegmentUtils::isTrailingWaste(int wasteLength, const QVector<Segment>& segments)
{
    if (segments.isEmpty())
        return false;

    const Segment& last = segments.last();
    return (last.type == SegmentType::Waste && last.length_mm == wasteLength);
}

