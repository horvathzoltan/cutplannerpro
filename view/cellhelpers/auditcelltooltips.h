#pragma once
#include "../../model/cutting/plan/cutplan.h"
#include "../../model/storageaudit/storageauditrow.h"
#include "materials/model/material_master.h"
#include "auditcelltext.h"
#include <QString>
#include "../../model/cutting/segment/segmentmodel.h"

namespace AuditCellTooltips {

// auditcelltooltips.h

inline QString forStatus(const StorageAuditRow& row, const MaterialMaster& mat) {
    QStringList lines;

    // 🔑 Emberi azonosítók
   // if (!row.rodId.isEmpty())
   //     lines << QString("RodId: %1").arg(row.rodId);
    lines << QString("Barcode: %1").arg(row.barcode.isEmpty() ? "—" : row.barcode);

    // 📦 Anyag és tároló
    lines << QString("Anyag: %1").arg(mat.name);
    if (!row.storageName.isEmpty())
        lines << QString("Tároló: %1").arg(row.storageName);

    // 📊 Context adatok
    if (row.hasContext()) {
        lines << QString("Auditcsoport: %1 (%2 tag)")
        .arg(row.groupKey())
            .arg(row.groupSize());
        lines << QString("Elvárt összesen: %1").arg(row.totalExpected());
        lines << QString("Tényleges összesen: %1").arg(row.totalActual());
        lines << QString("Hiányzó összesen: %1").arg(row.missingQuantity());
    }

    // 📋 Sor szintű adatok
    lines << QString("Elvárt (sor): %1").arg(row.totalExpected());
    lines << QString("Tényleges (sor): %1").arg(row.actualQuantity);
    lines << QString("Hiányzó (sor): %1").arg(row.missingQuantity());

    // 🟢 Státusz (suffixekkel együtt)
    lines << QString("Státusz: %1").arg(row.statusText());

    if (!row.isInOptimization)
        lines << "⚠️ Nem része az optimalizációnak";

    return lines.join("\n");
}

inline QString forSegment(const Cutting::Segment::SegmentModel& s,
                          const Cutting::Plan::CutPlan& plan,
                          const QString& groupName = "") {
    QStringList lines;

    lines << QString("RodId: %1").arg(plan.rodId);
    lines << QString("Barcode: %1").arg(s.barcode().isEmpty() ? "—" : s.barcode());
    lines << QString("Material: %1").arg(plan.materialName());
    lines << QString("Csoport: %1").arg(groupName.isEmpty() ? "—" : groupName);
    lines << QString("CutSize: %1 mm").arg(s.length_mm());

    if (s.isWaste())
        lines << QString("Remaining: %1 mm").arg(s.length_mm());
    else
        lines << "Remaining: —";

    QString sourceText = plan.source == Cutting::Plan::Source::Stock ? "MAT"
                         : plan.source == Cutting::Plan::Source::Reusable ? "RST"
                                                                          : "OPT";
    lines << QString("Forrás: %1").arg(sourceText);
    lines << QString("Gép: %1").arg(plan.machineName);
    lines << QString("Státusz: %1").arg(Cutting::Plan::statusText(plan.status));

    if (s.isWaste()) {
        lines << QString("Hulló azonosító: %1").arg(s.barcode().isEmpty() ? "—" : s.barcode());
        if (plan.parentBarcode.has_value()) {
            lines << QString("Forrás rúd: %1").arg(plan.parentBarcode.value());
        }
    }

    return lines.join("\n");
}



inline QString forExpected(const StorageAuditRow& row, const QString& groupLabel = "") {
    return AuditCellText::forExpected(row, groupLabel);
}

inline QString forMissing(const StorageAuditRow& row) {
    return AuditCellText::forMissing(row);
}


} // namespace AuditCellTooltips

