#pragma once

#include "common/tableutils/colorlogicutils.h"
#include "model/relocation/relocationinstruction.h"
#include "view/columnindexes/relocationplantable_columns.h"
#include "view/viewmodels/relocation/cellgenerator.h"
#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"
#include "view/cellhelpers/cellfactory.h"

#include <QColor>
#include <QObject>
#include <QPushButton>
#include <QHBoxLayout>
#include <QIcon>
#include <QStyle>

#include "common/styleprofiles/relocationcolors.h"

namespace Relocation::ViewModel::RowGenerator {

inline TableRowViewModel generateSumRow(const RelocationInstruction& instr) {

    TableRowViewModel vm;    
    vm.rowId = instr.rowId.isNull() ? QUuid::createUuid() : instr.rowId;

    // 🎨 Összesítő sor szürke háttérrel
    QColor bgColor = RelocationColors::SummaryBg; // sötétebb, egérszürke
    QColor fgColor = Qt::black;

    // Anyag
    vm.cells[RelocationPlanTableColumns::Material] =
        CellFactory::textCell(instr.materialName,
                              QString("Anyag: %1").arg(instr.materialName),
                              bgColor, fgColor);

    // Vonalkód
    vm.cells[RelocationPlanTableColumns::Barcode] =
        CellFactory::textCell("—",
                              "Összesítő sor, nincs vonalkód",
                              bgColor, fgColor);

    // Mennyiség
    QString qtyText = QString("%1/%2 (%3 maradék + %4 odavitt)")
                          .arg(instr.coveredQty)
                          .arg(instr.plannedQuantity)
                          .arg(instr.usedFromRemaining)
                          .arg(instr.movedQty);

    QColor qtyColor;
    if (instr.uncoveredQty > 0) {
        qtyColor = RelocationColors::Uncovered;
    } else if (instr.auditedRemaining < instr.totalRemaining) {
        qtyColor = RelocationColors::NotAudited;
    } else {
        qtyColor = RelocationColors::Covered;
    }

    vm.cells[RelocationPlanTableColumns::Quantity] =
        CellFactory::textCell(qtyText,
                              instr.summaryText,
                              bgColor, qtyColor);

    // Forrás / Cél
    vm.cells[RelocationPlanTableColumns::Source] =
        CellFactory::textCell("—", "Összesítő sor", bgColor, fgColor);
    vm.cells[RelocationPlanTableColumns::Target] =
        CellFactory::textCell("—", "Összesítő sor", bgColor, fgColor);

    // Típus
    vm.cells[RelocationPlanTableColumns::Type] =
        CellFactory::textCell("Σ Összesítő",
                              "Összesítő sor az igény lefedettségéről",
                              bgColor, fgColor);

    return vm;
}

/// 🔹 Teljes TableRowViewModel generálása egy RelocationInstruction alapján
inline TableRowViewModel generate(const RelocationInstruction& instr,
                                  const MaterialMaster* mat,
                                  QObject* receiver = nullptr) {

    TableRowViewModel vm;

    // Egységes rowId: ha az instruction nem ad id-t, generálunk egyet
    vm.rowId = instr.rowId.isNull() ? QUuid::createUuid() : instr.rowId;

    if (instr.isSummary) {
        // ha van külön summary generatorod, abban is állítsd be vm.rowId-t
        TableRowViewModel sum = generateSumRow(instr);
        sum.rowId = vm.rowId;
        return sum;
    }

    // 🎨 Alapszínek a csoport alapján
    QColor baseColor = ColorLogicUtils::resolveBaseColor(mat);
    QColor fgColor   = baseColor.lightness() < 128 ? Qt::white : Qt::black;


    // 🔹 Normál relocation sor (eredeti logika)
    vm.cells[RelocationPlanTableColumns::Material] =
        CellFactory::textCell(instr.materialName,
                              QString("Anyag: %1").arg(instr.materialName),
                              baseColor, fgColor);

    vm.cells[RelocationPlanTableColumns::Barcode] =
        CellFactory::textCell(instr.barcode,
                              QString("Vonalkód: %1").arg(instr.barcode),
                              baseColor, fgColor);

    QString qtyText = instr.isSatisfied
                          ? QStringLiteral("✔ Megvan")
                          : QString::number(instr.plannedQuantity);

    QColor qtyColor = instr.isSatisfied
                          ? QColor("#228B22")
                          : (instr.plannedQuantity == 0
                                 ? QColor("#B22222")
                                 : fgColor);

    vm.cells[RelocationPlanTableColumns::Quantity] =
        CellFactory::textCell(qtyText,
                              QString("Terv szerinti mennyiség: %1").arg(instr.plannedQuantity),
                              baseColor, qtyColor);

    QStringList sourceParts;
    for (const auto& src : instr.sources) {
        sourceParts << QString("%1 (%2/%3)")
        .arg(src.locationName)
            .arg(src.moved)
            .arg(src.available);
    }
    QString sourceText = sourceParts.isEmpty() ? "—" : sourceParts.join(", ");
    vm.cells[RelocationPlanTableColumns::Source] =
        // CellFactory::textCell(sourceText,
        //                       QString("Forrás tárhelyek: %1").arg(sourceText),
        //                       baseColor, fgColor);
        CellGenerator::createEditableCell(vm.rowId,
                                          sourceText,
                                          QString("Forrás tárhelyek: %1").arg(sourceText),
                                          receiver,
                                          "source");

    QStringList targetParts;
    for (const auto& tgt : instr.targets) {
        targetParts << QString("%1 (%2)")
        .arg(tgt.locationName)
            .arg(tgt.placed);
    }
    QString targetText = targetParts.isEmpty() ? "—" : targetParts.join(", ");
    vm.cells[RelocationPlanTableColumns::Target] =
        // CellFactory::textCell(targetText,
        //                       QString("Cél tárhelyek: %1").arg(targetText),
        //                       baseColor, fgColor);
        CellGenerator::createEditableCell(vm.rowId,
                                          targetText,
                                          QString("Cél tárhelyek: %1").arg(targetText),
                                          receiver,
                                          "target");

    QString typeText = (instr.sourceType == AuditSourceType::Stock)
                           ? QStringLiteral("📦 Stock")
                           : QStringLiteral("♻️ Hulló");
    vm.cells[RelocationPlanTableColumns::Type] =
        CellFactory::textCell(typeText,
                              QString("Forrás típusa: %1").arg(typeText),
                              baseColor, fgColor);

    return vm;
}

} // namespace RelocationRowViewModelGenerator
