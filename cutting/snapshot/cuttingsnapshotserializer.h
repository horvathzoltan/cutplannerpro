#pragma once

#include <QString>
#include <QTextStream>
#include <QVector>
#include <QUuid>

#include "model/cutting/plan/cutplan.h"
#include "model/cutting/segment/segmentmodel.h"
#include "model/cutting/piece/piecewithmaterial.h"

namespace Cutting {

class CuttingSnapshotSerializer
{
public:
    /**
     * @brief A teljes optimalizációs állapot mentése CSV snapshotba.
     * @param out A cél QTextStream (fájl vagy memória)
     * @param plans A CutPlan-ek listája
     */
    static void writeSnapshot(
        QTextStream& out,
        const QVector<Cutting::Plan::CutPlan>& plans);

private:
    static void writeHeader(QTextStream& out);
    static void writeCutPlans(QTextStream& out,
                              const QVector<Cutting::Plan::CutPlan>& plans);
    static void writeSegments(QTextStream& out,
                              const QVector<Cutting::Plan::CutPlan>& plans);
    static void writePieces(QTextStream& out,
                            const QVector<Cutting::Plan::CutPlan>& plans);
    static void writeLeftovers(QTextStream& out,
                               const QVector<Cutting::Plan::CutPlan>& plans);
};

} // namespace Cutting
