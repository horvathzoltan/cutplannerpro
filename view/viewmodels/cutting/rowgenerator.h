#pragma once

#include "common/styleprofiles/cuttingstatusutils.h"
#include "service/cutting/optimizer/optimizerconstants.h"
#include "view/cellhelpers/materialcellgenerator.h"
#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"
#include "view/columnindexes/tablecuttinginstruction_columns.h"
#include "model/cutting/instruction/cutinstruction.h"
#include "model/cutting/cuttingmachine.h"

#include <QObject>
#include <QPushButton>
#include <QColor>
#include <QUuid>

#include <model/registries/materialregistry.h>

/**
 * @brief CuttingInstruction RowGenerator
 *
 * Feladata:
 *  - Egy CutInstruction objektumból TableRowViewModel előállítása
 *  - Normál sor és Σ összesítő sor generálása
 *  - Színezés, tooltip, finalize gomb logika
 */
namespace Cutting::ViewModel::RowGenerator {

/// Összesítő sor generálása (Σ)
/// Gép szeparátor sor generálása
inline TableRowViewModel generateMachineSeparator(const MachineHeader& machine) {
    TableRowViewModel vm;
    vm.rowId = QUuid::createUuid();

    QColor bg = QColor("#B0C4DE"); // halvány kékesszürke háttér
    QColor fg = Qt::black;

    // Fő szöveg: gép neve
    QString text = QString("=== %1 ===").arg(machine.machineName);

    // Részletek: kerf, steller, kompenzáció
    QStringList details;
    details << QString("Kerf=%1 mm").arg(machine.kerf_mm, 0, 'f', 1);
    if (machine.stellerMaxLength_mm.has_value())
        details << QString("StellerMax=%1 mm").arg(machine.stellerMaxLength_mm.value(), 0, 'f', 1);
    if (machine.stellerCompensation_mm.has_value())
        details << QString("Comp=%1 mm").arg(machine.stellerCompensation_mm.value(), 0, 'f', 1);

    if (!details.isEmpty())
        text.append("   [" + details.join(" | ") + "]");

    if (!machine.comment.isEmpty())
        text.append(QString("   (%1)").arg(machine.comment));

    // Csak az első cellát töltjük ki, a többiben vizuális szeparátor
    vm.cells[CuttingInstructionTableColumns::StepId] =
        TableCellViewModel::fromText(text, "Gép szeparátor sor", bg, fg);

    for (int col = CuttingInstructionTableColumns::RodId;
         col <= CuttingInstructionTableColumns::Finalize; ++col) {
        vm.cells[col] = TableCellViewModel::fromText("—", "Separator", bg, fg, true);
    }

    return vm;
}

/// Normál sor generálása
inline TableRowViewModel generate(const CutInstruction& ci,
                                  const QColor& baseColor,
                                  QObject* receiver = nullptr) {
    TableRowViewModel vm;
    vm.rowId = ci.rowId.isNull() ? QUuid::createUuid() : ci.rowId;

    const auto* mat = MaterialRegistry::instance().findById(ci.materialId);


    // 🎨 Alapszínek a csoport alapján
    //QColor baseColor = ColorLogicUtils::resolveBaseColor(mat);
    QColor fgColor = baseColor.lightness() < 128 ? Qt::white : Qt::black;    bool done = (ci.status == CutStatus::Done);

    vm.cells[CuttingInstructionTableColumns::StepId] =
        TableCellViewModel::fromText(QString::number(ci.globalStepId), "Lépés azonosító", baseColor, fgColor);

    // RodLabel: marad a CutPlan által generált label
    vm.cells[CuttingInstructionTableColumns::RodId] =
        TableCellViewModel::fromText(ci.rodId, "Rúd jel", baseColor, fgColor);

    // Barcode: ha van konkrét rod barcode, azt mutatjuk, material megy tooltipbe
    QString barcodeToShow = ci.barcode.isEmpty() ? "—" : ci.barcode;
    QString barcodeTooltip = QString("Rod barcode: %1\nMaterial: %2")
                                 .arg(ci.barcode.isEmpty() ? "—" : ci.barcode)
                                 .arg(mat ? mat->name : "Ismeretlen");

    // vm.cells[CuttingInstructionTableColumns::Barcode] =
    //     TableCellViewModel::fromText(barcodeToShow, barcodeTooltip, baseColor, fgColor);

    vm.cells[CuttingInstructionTableColumns::Material] =
        CellGenerators::materialCell(ci.materialId, ci.barcode, baseColor, fgColor);

    QString cutText = QString("✂️ %1").arg(ci.cutSize_mm, 0, 'f', 1);
    QString cutTooltip = "Vágandó hossz (mm)";

    // kiemelt háttér és betű
    QColor cutBg = baseColor.darker(120);
    QColor cutFg = Qt::white;

    vm.cells[CuttingInstructionTableColumns::CutSize] =
        TableCellViewModel::fromText(
            cutText,
            cutTooltip,
            cutBg,
            cutFg,
            "font-weight: bold; font-size: 14px; text-decoration: underline;"
            );

    vm.cells[CuttingInstructionTableColumns::LengthBefore] =
        TableCellViewModel::fromText(QString::number(ci.lengthBefore_mm, 'f', 1),
                                     "Vágás előtti hossz (mm)", baseColor, fgColor);


    QColor fg = baseColor.lightness() < 128 ? Qt::white : Qt::black;
    QColor bg = baseColor; // alap háttérszín az anyag színe

    QString afterText = QString::number(ci.lengthAfter_mm, 'f', 1);
    QString afterTooltip = "Vágás utáni hossz (mm)";

    if (ci.isFinalLeftover && ci.lengthAfter_mm > 0) {
        double len = ci.lengthAfter_mm;

        if (len < OptimizerConstants::SELEJT_THRESHOLD) {
            bg = QColor("#e74c3c"); // selejt → piros
            fg = Qt::white;
        } else if (len >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
                   len <= OptimizerConstants::GOOD_LEFTOVER_MAX) {
            bg = QColor("#f1c40f"); // jó leftover → sárga
            fg = Qt::black;
        } else {
            bg = QColor("#e67e22"); // köztes leftover → narancs
            fg = Qt::white;
        }

        afterText.append(QString("  [%1]").arg(ci.leftoverBarcode));
        afterTooltip = QString("Végső leftover (%1 mm, kategória: %2)")
                           .arg(len)
                           .arg(len < OptimizerConstants::SELEJT_THRESHOLD ? "Selejt"
                                : (len >= OptimizerConstants::GOOD_LEFTOVER_MIN &&
                                   len <= OptimizerConstants::GOOD_LEFTOVER_MAX) ? "Jó"
                                                                                : "Köztes");
    }

    vm.cells[CuttingInstructionTableColumns::LengthAfter] =
        TableCellViewModel::fromText(afterText, afterTooltip, bg, fg);



    vm.cells[CuttingInstructionTableColumns::Status] =
        TableCellViewModel::fromText(
            CuttingStatusUtils::toText(ci.status),
            "Vágási státusz",
            baseColor,
            CuttingStatusUtils::toColor(ci.status)
            );

    if (done) {
        vm.cells[CuttingInstructionTableColumns::Finalize] =
            TableCellViewModel::fromText("✔", "Már végrehajtva", baseColor, QColor("#666666"), true);
    } else {
        QPushButton* btn = new QPushButton("Finalize");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
        btn->setToolTip("A vágás végrehajtása és leftover regisztrálása.");
        QObject::connect(btn, &QPushButton::clicked, receiver, [receiver, rowId = ci.rowId]() {
            QMetaObject::invokeMethod(receiver, "finalizeRow", Qt::QueuedConnection,
                                      Q_ARG(QUuid, rowId));
        });
        vm.cells[CuttingInstructionTableColumns::Finalize] =
            TableCellViewModel::fromWidget(btn, "Végrehajtás");
    }

    return vm;
}

} // namespace Cutting::ViewModel::RowGenerator
