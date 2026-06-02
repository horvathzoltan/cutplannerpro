#pragma once

#include <QHash>
#include <QVector>
#include <materials/registry/material_registry.h>
#include "cuttypes.h"
#include "../../../service/cutting/optimizer/optimizerutils.h"
#include "../../../common/logger.h"

namespace Cutting {
namespace Optimizer {

struct PendingStats
{
    QUuid targetMaterialId;
    int totalPending = 0;
    int totalLength = 0;
    QUuid maxMat;
    int maxCount = 0;
};

class PendingAnalyzer
{
public:
    static inline PendingStats analyze(
        const QHash<QUuid, QVector<Cutting::Piece::PieceWithMaterial>>& piecesByMaterial,
        TargetHeuristic heuristic)
    {
        PendingStats out;

        int bestMetric = -1;

        for (auto it = piecesByMaterial.begin(); it != piecesByMaterial.end(); ++it)
        {
            const QUuid& matId = it.key();
            const auto& vec = it.value();

            if (vec.isEmpty())
                continue;

            int count = vec.size();
            out.totalPending += count;

            auto info = OptimizerUtils::computePhysicalCut(vec, 0.0, INT_MAX);
            int sumLen = info.totalCut;
            out.totalLength += sumLen;

            if (count > out.maxCount) {
                out.maxCount = count;
                out.maxMat = matId;
            }

            double metric = (heuristic == TargetHeuristic::ByCount)
                                ? count
                                : sumLen;

            if (metric > bestMetric) {
                bestMetric = metric;
                out.targetMaterialId = matId;
            }

            const MaterialMaster* mm = MaterialRegistry::instance().findById(matId);
            zInfo(QString("   • Anyag=%1 → %2 db, összhossz=%3 mm")
                      .arg(mm ? mm->toDisplay() : matId.toString())
                      .arg(count)
                      .arg(sumLen));
        }

        const MaterialMaster* maxMatPtr =
            MaterialRegistry::instance().findById(out.maxMat);

        zInfo(QString("📊 Pending összegzés — összes=%1 db, összhossz=%2 mm, "
                      "legnagyobb anyagcsoport=%3 (%4 db)")
                  .arg(out.totalPending)
                  .arg(out.totalLength)
                  .arg(maxMatPtr ? maxMatPtr->toDisplay() : out.maxMat.toString())
                  .arg(out.maxCount));

        return out;
    }
};

} // namespace Optimizer
} // namespace Cutting
