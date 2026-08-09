#pragma once

#include <QString>
#include <QTextStream>
#include <QVector>
#include <QUuid>
#include <QMap>

#include "model/cutting/plan/cutplan.h"
#include "model/cutting/segment/segmentmodel.h"
#include "model/cutting/piece/piecewithmaterial.h"

namespace Cutting {

class CuttingSnapshotDeserializer
{
public:
    /**
     * @brief Snapshot visszatöltése CSV-ből.
     * @param in A bemeneti QTextStream
     * @return A visszaállított CutPlan-ek listája
     */
    static QVector<Cutting::Plan::CutPlan> loadSnapshot(QTextStream& in);

private:
    enum Section {
        NONE,
        CUTPLANS,
        SEGMENTS,
        PIECES,
        LEFTOVERS
    };

    static Section detectSection(const QString& line);

    static void parseCutPlan(
        const QStringList& parts,
        QVector<Cutting::Plan::CutPlan>& plans);

    static void parseSegment(
        const QStringList& parts,
        QMap<QUuid, QVector<Cutting::Segment::SegmentModel>>& segmentsByPlan);

    static void parsePiece(
        const QStringList& parts,
        QMap<QUuid, QVector<Cutting::Piece::PieceWithMaterial>>& piecesByPlan);

    static void parseLeftover(
        const QStringList& parts,
        QMap<QUuid, QString>& leftoverByPlan,
        QMap<QUuid, double>& leftoverWasteByPlan);

    static void finalizePlans(
        QVector<Cutting::Plan::CutPlan>& plans,
        const QMap<QUuid, QVector<Cutting::Segment::SegmentModel>>& segmentsByPlan,
        const QMap<QUuid, QVector<Cutting::Piece::PieceWithMaterial>>& piecesByPlan,
        const QMap<QUuid, QString>& leftoverByPlan,
        const QMap<QUuid, double>& leftoverWasteByPlan);
};

} // namespace Cutting
