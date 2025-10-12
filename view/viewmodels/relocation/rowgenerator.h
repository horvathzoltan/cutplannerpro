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
        TableCellViewModel::fromText(instr.materialName,
                              QString("Anyag: %1").arg(instr.materialName),
                              bgColor, fgColor);

    // Vonalkód
    vm.cells[RelocationPlanTableColumns::Barcode] =
        TableCellViewModel::fromText("—",
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
        TableCellViewModel::fromText(qtyText,
                              instr.summaryText,
                              bgColor, qtyColor);

    // Forrás / Cél
    vm.cells[RelocationPlanTableColumns::Source] =
        TableCellViewModel::fromText("—", "Összesítő sor", bgColor, fgColor);
    vm.cells[RelocationPlanTableColumns::Target] =
        TableCellViewModel::fromText("—", "Összesítő sor", bgColor, fgColor);

    // Típus
    vm.cells[RelocationPlanTableColumns::Type] =
        TableCellViewModel::fromText("Σ Összesítő",
                              "Összesítő sor az igény lefedettségéről",
                              bgColor, fgColor);

    return vm;
}

/// 🔹 Teljes TableRowViewModel generálása egy RelocationInstruction alapján
inline TableRowViewModel generate(const RelocationInstruction& instr,
                                  const MaterialMaster* mat,
                                  QObject* receiver = nullptr) {

    TableRowViewModel vm;

    vm.rowId = instr.rowId.isNull() ? QUuid::createUuid() : instr.rowId;

    if (instr.isSummary) {
        TableRowViewModel sum = generateSumRow(instr);
        sum.rowId = vm.rowId;
        return sum;
    }

    QColor baseColor = ColorLogicUtils::resolveBaseColor(mat);
    QColor fgColor   = baseColor.lightness() < 128 ? Qt::white : Qt::black;

    vm.cells[RelocationPlanTableColumns::Material] =
        TableCellViewModel::fromText(instr.materialName,
                              QString("Anyag: %1").arg(instr.materialName),
                              baseColor, fgColor);

    vm.cells[RelocationPlanTableColumns::Barcode] =
        TableCellViewModel::fromText(instr.barcode,
                              QString("Vonalkód: %1").arg(instr.barcode),
                              baseColor, fgColor);

    QString qtyText = instr.isSatisfied
                          ? QStringLiteral("✔ Megvan")
                          : QString::number(instr.plannedQuantity);

    QColor qtyColor = instr.isSatisfied
                          ? QColor("#228B22")
                          : (instr.plannedQuantity == 0 ? QColor("#B22222") : fgColor);

    vm.cells[RelocationPlanTableColumns::Quantity] =
        TableCellViewModel::fromText(qtyText,
                              QString("Terv szerinti mennyiség: %1").arg(instr.plannedQuantity),
                              baseColor, qtyColor);

    if (instr.sourceType == AuditSourceType::Stock) {
        QStringList sourceParts;
        for (const auto& src : instr.sources) {
            sourceParts << QString("%1 (%2/%3)")
            .arg(src.locationName)
                .arg(src.moved)
                .arg(src.available);
        }
        QString sourceText = sourceParts.isEmpty() ? QStringLiteral("—") : sourceParts.join(", ");
        vm.cells[RelocationPlanTableColumns::Source] =
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
        QString targetText = targetParts.isEmpty() ? QStringLiteral("—") : targetParts.join(", ");
        vm.cells[RelocationPlanTableColumns::Target] =
            CellGenerator::createEditableCell(vm.rowId,
                                              targetText,
                                              QString("Cél tárhelyek: %1").arg(targetText),
                                              receiver,
                                              "target");
    } else {
        QString sourceText = instr.sources.isEmpty() ? QStringLiteral("—") : instr.sources.first().locationName;
        vm.cells[RelocationPlanTableColumns::Source] =
            TableCellViewModel::fromText(sourceText,
                                  QString("Hulló forrás: %1").arg(sourceText),
                                  baseColor, fgColor);

        vm.cells[RelocationPlanTableColumns::Target] =
            TableCellViewModel::fromText(QStringLiteral("—"),
                                  QStringLiteral("Hullónál nincs cél"),
                                  baseColor, fgColor);
    }

    QString typeText = (instr.sourceType == AuditSourceType::Stock)
                           ? QStringLiteral("📦 Stock")
                           : QStringLiteral("♻️ Hulló");
    vm.cells[RelocationPlanTableColumns::Type] =
        TableCellViewModel::fromText(typeText,
                              QString("Forrás típusa: %1").arg(typeText),
                              baseColor, fgColor);

    // Finalize gomb cella (frissített logika)
    if (instr.sourceType == AuditSourceType::Stock) {
        QPushButton* btn = new QPushButton("Finalize");
        btn->setCursor(Qt::PointingHandCursor);

        qInfo() << "Row" << instr.rowId
                << "isFinalized=" << instr.isAlreadyFinalized()
                << "isLeftover=" << instr.isLeftover()
                << "hasTarget=" << instr.hasTarget()
                << "targets.size=" << instr.targets.size()
                << "planned=" << instr.plannedQuantity
                << "available=" << instr.availableQuantity();

        if (instr.isAlreadyFinalized()) {
            btn->setText("✔");
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: #ccc; color: #666;");
            btn->setToolTip(QStringLiteral("A sor már véglegesítve lett."));
        } else if (instr.isLeftover()) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: #eee; color: #999;");
            btn->setToolTip(QStringLiteral("Hulladékot nem finalizálunk."));
        } else if (!instr.hasTarget()) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: #eee; color: #999;");
            btn->setToolTip(QStringLiteral("Nincs cél. Hozz létre célt vagy válassz meglévőt a sor szerkesztésével."));
        } else if (!instr.isReadyToFinalize()) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: #eee; color: #999;");
            btn->setToolTip(QStringLiteral("A forrás nem tartalmaz elegendő mennyiséget a véglegesítéshez."));
        } else {
            btn->setEnabled(true);
            btn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
            btn->setToolTip(QStringLiteral("A sor véglegesíthető."));
        }

        QObject::connect(btn, &QPushButton::clicked, receiver, [receiver, rowId = instr.rowId]() {
            QMetaObject::invokeMethod(receiver, "finalizeRow", Qt::QueuedConnection,
                                      Q_ARG(QUuid, rowId));
        });

        vm.cells[RelocationPlanTableColumns::Finalize] =
            TableCellViewModel::fromWidget(btn, QStringLiteral("Finalize gomb"));
    } else {
        // Hullónál ne legyen Finalize gomb — üres, nem interaktív cella
        vm.cells[RelocationPlanTableColumns::Finalize] =
            TableCellViewModel::fromText("-",
                                  QStringLiteral("Nincs finalize hullónál"),
                                  baseColor, fgColor);
    }


    return vm;
}



} // namespace RelocationRowViewModelGenerator
