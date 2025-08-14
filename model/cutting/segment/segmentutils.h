#pragma once

#include "model/cutting/piece/piecewithmaterial.h"
#include "segmentmodel.h"
#include <QVector>

namespace Cutting{
namespace Segment{


namespace SegmentUtils {
//QVector<Segment> generateSegments(const QVector<int>& cuts, int kerf_mm, int totalLength_mm);

/**
     * @brief Megállapítja, hogy a megadott hulladék a vágási terv végmaradéka-e
     */
bool isTrailingWaste(int wasteLength, const QVector<SegmentModel>& segments);

QVector<SegmentModel> generateSegments(const QVector<Cutting::Piece::PieceWithMaterial>& cuts, int kerf_mm, int totalLength_mm);

}

}}
