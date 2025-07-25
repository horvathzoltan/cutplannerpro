#pragma once

#include "../model/segment.h"
#include "model/cutting/piecewithmaterial.h"
#include <QVector>

namespace SegmentUtils {
//QVector<Segment> generateSegments(const QVector<int>& cuts, int kerf_mm, int totalLength_mm);

/**
     * @brief Megállapítja, hogy a megadott hulladék a vágási terv végmaradéka-e
     */
bool isTrailingWaste(int wasteLength, const QVector<Segment>& segments);

QVector<Segment> generateSegments(const QVector<PieceWithMaterial>& cuts, int kerf_mm, int totalLength_mm);

}
