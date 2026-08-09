#include "cuttingsnapshotdeserializer.h"

#include "materials/registry/material_registry.h"
#include "model/registries/cuttingmachineregistry.h"

#include <model/registries/cuttingplanrequestregistry.h>

namespace Cutting {

CuttingSnapshotDeserializer::Section
CuttingSnapshotDeserializer::detectSection(const QString& line)
{
    if (line.startsWith("#CUTPLANS")) return CUTPLANS;
    if (line.startsWith("#SEGMENTS")) return SEGMENTS;
    if (line.startsWith("#PIECES"))   return PIECES;
    if (line.startsWith("#LEFTOVERS"))return LEFTOVERS;
    return NONE;
}

QVector<Cutting::Plan::CutPlan>
CuttingSnapshotDeserializer::loadSnapshot(QTextStream& in)
{
    QVector<Cutting::Plan::CutPlan> plans;

    QMap<QUuid, QVector<Cutting::Segment::SegmentModel>> segmentsByPlan;
    QMap<QUuid, QVector<Cutting::Piece::PieceWithMaterial>> piecesByPlan;
    QMap<QUuid, QString> leftoverByPlan;
    QMap<QUuid, double> leftoverWasteByPlan;

    Section current = NONE;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("#")) {
            current = detectSection(line);
            continue;
        }

        QStringList parts = line.split(';');

        switch (current) {
        case CUTPLANS:
            parseCutPlan(parts, plans);
            break;

        case SEGMENTS:
            parseSegment(parts, segmentsByPlan);
            break;

        case PIECES:
            parsePiece(parts, piecesByPlan);
            break;

        case LEFTOVERS:
            parseLeftover(parts, leftoverByPlan, leftoverWasteByPlan);
            break;

        default:
            break;
        }
    }

    finalizePlans(plans, segmentsByPlan, piecesByPlan, leftoverByPlan, leftoverWasteByPlan);
    return plans;
}

void CuttingSnapshotDeserializer::parseCutPlan(
    const QStringList& parts,
    QVector<Cutting::Plan::CutPlan>& plans)
{
    Cutting::Plan::CutPlan plan;

    plan.planId = QUuid(parts[0]);
    plan.planNumber = parts[1].toInt();
    plan.status = Cutting::Plan::StatusUtils::fromCsv(parts[2]);
    plan.rodId = parts[3];

    // materialBarcode → registry lookup
    if (auto* mat = MaterialRegistry::instance().findByBarcode(parts[4])) {
        plan.materialId = mat->id;
    }

    // machineBarcode → registry lookup
    if (auto* machine = CuttingMachineRegistry::instance().findByBarcode(parts[5])) {
        plan.machineId = machine->id;
        plan.machineName = machine->name;
    }

    plan.machineKerf = parts[6].toDouble();
    plan.source = Cutting::Plan::SourceUtils::fromCsv(parts[7]);
    plan.sourceBarcode = parts[8];
    plan.optimizationId = parts[9].toInt();
    plan.toldasRole = ToldasRoleUtils::fromCsv(parts[10]);

    // ParentInfo CSV (barcode;planId)
    if (parts.size() > 11 && !parts[11].isEmpty()) {
        QStringList p = parts[11].split(';');
        QString parentBarcode = p.value(0);
        QString parentPlanIdStr = p.value(1);

        plan.setParent(
            Cutting::Plan::ParentInfo::fromCsv(parentBarcode, parentPlanIdStr)
            );
    }

    if (parts.size() > 12) {
        plan._segments.SetTotalLength_mm(parts[12].toInt());
    }

    plans.append(plan);
}

void CuttingSnapshotDeserializer::parseSegment(
    const QStringList& parts,
    QMap<QUuid, QVector<Cutting::Segment::SegmentModel>>& segmentsByPlan)
{
    QUuid pid(parts[0]);
    int segIndex = parts[1].toInt();
    QString typeStr = parts[2];
    double length = parts[3].toDouble();
    QString barcode = parts[4];
    QUuid pieceId(parts[5]);
    QString externalRef = parts[6];

    Cutting::Segment::SegmentModel seg;
    seg._segId = QUuid::createUuid();     // új ID, snapshot nem tárolja
    seg._type = Cutting::Segment::SegmentModel::segmentTypeFromPrefix(typeStr);
    seg._length_mm = length;
    seg._barcode = barcode;               // snapshotból jön
    seg._pieceId = pieceId;
    seg.externalReference = externalRef;

    // auto r = CuttingPlanRequestRegistry::instance().findByExtRefAndMaterial(seg.externalReference, p.materialId);
    // if(r){
    //     seg._requestId = r->requestId;
    // }

    segmentsByPlan[pid].append(seg);
}


void CuttingSnapshotDeserializer::parsePiece(
    const QStringList& parts,
    QMap<QUuid, QVector<Cutting::Piece::PieceWithMaterial>>& piecesByPlan)
{
    QUuid pid(parts[0]);
    QUuid pieceId(parts[1]);
    double length = parts[2].toDouble();
    QString matBarcode = parts[3];
    QString externalRef = parts[4];

    Cutting::Piece::PieceWithMaterial p;

    // --- PieceInfo mezők ---
    p.info.pieceId = pieceId;
    p.info.length_mm = static_cast<int>(length);
    p.info.externalReference = externalRef;

    // --- materialBarcode → materialId ---
    auto* mat = MaterialRegistry::instance().findByBarcode(matBarcode);
    if (mat) {
        p.materialId = mat->id;
    } else {
        p.materialId = QUuid(); // fallback
    }
    // 🔥 REQUEST HOZZÁRENDELÉSE
    if (mat) {
        auto* req = CuttingPlanRequestRegistry::instance()
        .findByExtRefAndMaterial(externalRef, mat->id);

        if (req) {
            p.info.requestId = req->requestId;           // 🔥 requestId
            p.productTypeId = req->productTypeId;        // 🔥 termék típus
            p.productSubtypeId = req->productSubtypeId;  // 🔥 termék altípus
            p.attributes = req->attributes;              // 🔥 attribútumok
            p.side = (req->leftCount > 0 && req->rightCount == 0)
                         ? HandlerSide::Left
                         : (req->rightCount > 0 && req->leftCount == 0)
                               ? HandlerSide::Right
                               : HandlerSide::None;
        }
    }


    // A többi mező snapshotban nincs → default érték marad
    // p.info.requestId
    // p.info.isCompleted
    // p.info.leftoverEntryId
    // p.info.toldasRole
    // p.info.keepWhole
    // p.productTypeId
    // p.productSubtypeId
    // p.attributes
    // p.side
    // p.failed
    // p.failReason

    piecesByPlan[pid].append(p);
}


void CuttingSnapshotDeserializer::parseLeftover(
    const QStringList& parts,
    QMap<QUuid, QString>& leftoverByPlan,
    QMap<QUuid, double>& leftoverWasteByPlan)
{
    QUuid pid(parts[0]);
    leftoverByPlan[pid] = parts[1];
    leftoverWasteByPlan[pid] = parts[2].toDouble();
}

void CuttingSnapshotDeserializer::finalizePlans(
    QVector<Cutting::Plan::CutPlan>& plans,
    const QMap<QUuid, QVector<Cutting::Segment::SegmentModel>>& segmentsByPlan,
    const QMap<QUuid, QVector<Cutting::Piece::PieceWithMaterial>>& piecesByPlan,
    const QMap<QUuid, QString>& leftoverByPlan,
    const QMap<QUuid, double>& leftoverWasteByPlan)
{
    for (auto& plan : plans) {



        // --- SEGMENTS ---
        if (segmentsByPlan.contains(plan.planId)) {
            auto segments =segmentsByPlan[plan.planId];
            for(auto&s:segments){
                for(auto&p:piecesByPlan[plan.planId]){
                    if(p.info.pieceId == s._pieceId){
                        s._requestId = p.info.requestId;
                        break;
                    }
                }
            }

            plan._segments.setSegments(segments);
        }

        // --- PIECES ---
        if (piecesByPlan.contains(plan.planId)) {
            plan.piecesWithMaterial = piecesByPlan[plan.planId];
        }

        // --- LEFTOVER ---
        if (leftoverByPlan.contains(plan.planId)) {
            plan._segments.restoreLeftoverBarcode(leftoverByPlan[plan.planId]);
            // waste_mm() számított érték → nem kell visszaállítani
        }
    }
}


} // namespace Cutting
