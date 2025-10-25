#pragma once

//#include "common/styleprofiles/cuttingcolors.h"
#include "common/styleprofiles/cuttingstatusutils.h"
#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"
#include "view/columnindexes/tablecuttinginstruction_columns.h"
#include "model/cutting/instruction/cutinstruction.h"
//#include "view/tableutils/colorlogicutils.h"
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
inline TableRowViewModel generateMachineSeparator(const CuttingMachine& machine) {
    TableRowViewModel vm;
    vm.rowId = QUuid::createUuid();

    QColor bg = QColor("#B0C4DE"); // halvány kékesszürke háttér
    QColor fg = Qt::black;

    // Fő szöveg: gép neve
    QString text = QString("=== %1 ===").arg(machine.name);

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

    for (int col = CuttingInstructionTableColumns::RodLabel;
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
        TableCellViewModel::fromText(QString::number(ci.stepId), "Lépés azonosító", baseColor, fgColor);
    vm.cells[CuttingInstructionTableColumns::RodLabel] =
        TableCellViewModel::fromText(ci.rodLabel, "Rúd jel", baseColor, fgColor);
    vm.cells[CuttingInstructionTableColumns::Material] =
        TableCellViewModel::fromText(mat ? mat->name : "Ismeretlen", "Anyag", baseColor, fgColor);
    vm.cells[CuttingInstructionTableColumns::Barcode] =
        TableCellViewModel::fromText(ci.barcode, "Vonalkód", baseColor, fgColor);

    vm.cells[CuttingInstructionTableColumns::CutSize] =
        TableCellViewModel::fromText(QString::number(ci.cutSize_mm, 'f', 1),
                                     "Vágandó hossz (mm)", baseColor, fgColor);
    vm.cells[CuttingInstructionTableColumns::RemainingBefore] =
        TableCellViewModel::fromText(QString::number(ci.remainingBefore_mm, 'f', 1),
                                     "Vágás előtti hossz (mm)", baseColor, fgColor);
    vm.cells[CuttingInstructionTableColumns::RemainingAfter] =
        TableCellViewModel::fromText(QString::number(ci.remainingAfter_mm, 'f', 1),
                                     "Vágás utáni hossz (mm)", baseColor, fgColor);

    vm.cells[CuttingInstructionTableColumns::Machine] =
        TableCellViewModel::fromText("Krokodil", "Gép neve", baseColor, fgColor);


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
