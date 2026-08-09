#include "cuttingsnapshotserializer.h"

#include "model/cutting/cuttingmachine.h"
#include "materials/registry/material_registry.h"
#include "model/registries/cuttingmachineregistry.h"


namespace Cutting {

void CuttingSnapshotSerializer::writeSnapshot(
    QTextStream& out,
    const QVector<Cutting::Plan::CutPlan>& plans)
{
    writeHeader(out);
    writeCutPlans(out, plans);
    writeSegments(out, plans);
    writePieces(out, plans);
    writeLeftovers(out, plans);
}

void CuttingSnapshotSerializer::writeHeader(QTextStream& out)
{
    out << "#VERSION 2\n";
}

void CuttingSnapshotSerializer::writeCutPlans(
    QTextStream& out,
    const QVector<Cutting::Plan::CutPlan>& plans)
{
    out << "#CUTPLANS\n";

    for (const auto& plan : plans) {

        auto* machine = CuttingMachineRegistry::instance().findById(plan.machineId);
        if (!machine) continue;

        auto* mat = MaterialRegistry::instance().findById(plan.materialId);
        if (!mat) continue;

        out << plan.planId.toString() << ";"
            << plan.planNumber << ";"
            << Cutting::Plan::StatusUtils::toCsv(plan.status) << ";"
            << plan.rodId << ";"
            << mat->barcode << ";"
            << machine->barcode << ";"
            << plan.machineKerf << ";"
            << Cutting::Plan::SourceUtils::toCsv(plan.source) << ";"
            << plan.sourceBarcode << ";"
            << plan.optimizationId << ";"
            << ToldasRoleUtils::toCsv(plan.toldasRole) << ";"
            << (plan.parent().has_value()
                    ? plan.parent()->toCsv()
                    : "")
            << plan._segments.totalLength_mm()
            << "\n";
    }
}

void CuttingSnapshotSerializer::writeSegments(
    QTextStream& out,
    const QVector<Cutting::Plan::CutPlan>& plans)
{
    out << "#SEGMENTS\n";

    for (const auto& plan : plans) {
        for (int i = 0; i < plan._segments.size(); ++i) {
            const auto& seg = plan._segments.segment(i);

            out << plan.planId.toString() << ";"
                << i << ";"
                << Cutting::Segment::SegmentModel::segmentPrefix(seg._type) << ";"
                << seg.length_mm() << ";"
                << seg.barcode() << ";"
                << seg._pieceId.toString() << ";"
                << seg.externalReference
                << "\n";
        }
    }
}

void CuttingSnapshotSerializer::writePieces(
    QTextStream& out,
    const QVector<Cutting::Plan::CutPlan>& plans)
{
    out << "#PIECES\n";

    for (const auto& plan : plans) {
        for (const auto& p : plan.piecesWithMaterial) {

            // materialBarcode → registryből kell kinyerni
            QString matBarcode;
            if (auto* mat = MaterialRegistry::instance().findById(p.materialId)) {
                matBarcode = mat->barcode;
            }

            out << plan.planId.toString() << ";"
                << p.info.pieceId.toString() << ";"
                << p.info.length_mm << ";"
                << matBarcode << ";"
                << p.info.externalReference
                << "\n";
        }
    }
}


void CuttingSnapshotSerializer::writeLeftovers(
    QTextStream& out,
    const QVector<Cutting::Plan::CutPlan>& plans)
{
    out << "#LEFTOVERS\n";

    for (const auto& plan : plans) {
        QString bc = plan._segments.leftoverBarcode();
        if (bc.isEmpty()) continue;

        out << plan.planId.toString() << ";"
            << bc << ";"
            << plan._segments.waste_mm()
            << "\n";
    }
}

} // namespace Cutting
