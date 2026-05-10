#include "cutplan.h"
#include <QDebug>
#include "materials/registry/material_registry.h"
#include "materials/utils/material_group_utils.h"
#include <model/registries/cuttingplanrequestregistry.h>

namespace Cutting {
namespace Plan {

bool CutPlan::isReusable() const
{
        return source == Cutting::Plan::Source::Reusable;
}

bool CutPlan::isFinalized() const
{
    // Completed vagy manuálisan lezárt (Abandoned) tervek már nem módosíthatók
    return status == Status::Completed || status == Status::Abandoned;
}

QString CutPlan::materialName() const
{
    // MaterialRegistry-ből lekérjük az anyag nevét
    auto opt = MaterialRegistry::instance().findById(materialId);
    return opt ? opt->name : "(?)";
}

QString CutPlan::materialBarcode() const
{
    // MaterialRegistry-ből lekérjük az anyag nevét
    auto opt = MaterialRegistry::instance().findById(materialId);
    return opt ? opt->barcode : "(?)";
}

QString CutPlan::materialGroupName() const
{
    // Csoportnév az anyag ID alapján — helperből
    return GroupUtils::groupName(materialId);
}

Status CutPlan::getStatus() const
{
    return status;
}

void CutPlan::setStatus(Status newStatus)
{
    status = newStatus;
}

QString CutPlan::pieceLengthsAsString() const {
    QStringList out;
    for (const Cutting::Segment::SegmentModel& s : segments) {
        if (s.type() == Cutting::Segment::SegmentModel::Type::Piece)
            out << s.length_txt();
    }
    return out.join(";");
}


QString CutPlan::toLogEntry(const CuttingMachine& machine) const
{
    QString sourceLabel;
    if (source == Cutting::Plan::Source::Reusable) {
        sourceLabel = QString("Hulló: %1").arg(sourceBarcode);
    } else {
        const MaterialMaster* mat = MaterialRegistry::instance().findById(materialId);
        sourceLabel = mat ? QString("Anyag: %1").arg(mat->name)
                          : QString("Anyag: ? (%1)").arg(materialId.toString());
    }

    struct PieceAgg { int count = 0; QStringList refs; };
    QMap<int, PieceAgg> agg;

    for (const auto& pw : piecesWithMaterial) {
        int len = pw.info.length_mm;
        agg[len].count += 1;

        auto req = CuttingPlanRequestRegistry::instance().findById(pw.info.requestId);
        QString ref = req ? req->externalReference : "???";
        agg[len].refs.append(ref);
    }

    QStringList pieceList;
    for (auto it = agg.begin(); it != agg.end(); ++it) {
        QString refs = it.value().refs.join(", ");
        pieceList << QString("%1×%2 mm [%3]")
                         .arg(it.value().count)
                         .arg(it.key())
                         .arg(refs);
    }

    return QString("🪚 CutPlan #%1 → %2, Rod=%3, gép=%4, kerf=%5 mm\n%6\nhulladék=%7 mm")
        .arg(planNumber)
        .arg(sourceLabel)
        .arg(rodId)
        .arg(machine.name)
        .arg(QString::number(kerfUsed_mm, 'f', 1))
        .arg(pieceList.join(", "))
        .arg(waste);
}


} //endof namespace Plan
} //endof namespace Cutting

