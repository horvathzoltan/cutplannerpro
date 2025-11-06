#pragma once

#include "common/styleprofiles/cuttingstatusutils.h"
#include "service/cutting/optimizer/optimizerconstants.h"
#include "view/cellhelpers/materialcellgenerator.h"
#include "view/viewmodels/tablerowviewmodel.h"
#include "view/viewmodels/tablecellviewmodel.h"
#include "view/columnindexes/tablecuttinginstruction_columns.h"
#include "model/cutting/instruction/cutinstruction.h"

#include <QObject>
#include <QPushButton>
#include <QColor>
#include <QUuid>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>

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
inline TableRowViewModel generateMachineSeparator(const MachineHeader& machine,
                                                  QObject* receiver = nullptr) {
    TableRowViewModel vm;
    vm.rowId = QUuid::createUuid();

    QColor bg = QColor("#B0C4DE"); // halvány kékesszürke háttér
    QColor fg = Qt::black;

    // 🔹 Composite widget a badge-szerű megjelenítéshez
    QWidget* badgeWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(badgeWidget);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    // Gépnév badge
    QLabel* lblMachine = new QLabel(QString("=== %1 ===").arg(machine.machineName));
    lblMachine->setStyleSheet("QLabel { background-color: #2980b9; color: white; font-weight: bold; padding: 4px 10px; border-radius: 6px; }");
    layout->addWidget(lblMachine);

    // Kerf badge
    QLabel* lblKerf = new QLabel(QString("Kerf=%1 mm").arg(machine.kerf_mm, 0, 'f', 1));
    lblKerf->setStyleSheet("QLabel { background-color: #16a085; color: white; padding: 3px 8px; border-radius: 6px; }");
    layout->addWidget(lblKerf);

    // StellerMax badge (ha van)
    if (machine.stellerMaxLength_mm.has_value()) {
        QLabel* lblSteller = new QLabel(QString("StellerMax=%1 mm").arg(machine.stellerMaxLength_mm.value(), 0, 'f', 1));
        lblSteller->setStyleSheet("QLabel { background-color: #8e44ad; color: white; padding: 3px 8px; border-radius: 6px; }");
        layout->addWidget(lblSteller);
    }

    // Kompenzáció badge-szerű widget: label + spinbox + suffix
    if (machine.stellerMaxLength_mm.has_value() && machine.stellerMaxLength_mm.value() > 0) {
        QWidget* compWidget = new QWidget();
        QHBoxLayout* compLayout = new QHBoxLayout(compWidget);
        compLayout->setContentsMargins(0,0,0,0);
        compLayout->setSpacing(4);

        QLabel* lblPrefix = new QLabel("Comp:");
        lblPrefix->setStyleSheet("font-weight: bold; color: #2c3e50;");

        QDoubleSpinBox* spin = new QDoubleSpinBox();
        spin->setRange(-50.0, 50.0);
        spin->setDecimals(2);
        spin->setValue(machine.stellerCompensation_mm.value_or(0.0));
        spin->setToolTip("Steller kompenzáció felülírása");
        spin->setKeyboardTracking(false);
        spin->setLocale(QLocale(QLocale::Hungarian)); // magyar tizedeselválasztó

        // 🔑 Bekötés: amikor változik az érték
        QObject::connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         receiver, [receiver, machineId = machine.machineId](double newVal) {
                             QMetaObject::invokeMethod(receiver, "compensationChange",
                                                       Qt::QueuedConnection,
                                                       Q_ARG(QUuid, machineId),
                                                       Q_ARG(double, newVal));
                         });

        QLabel* lblSuffix = new QLabel("mm");
        lblSuffix->setStyleSheet("color: #2c3e50;");

        compLayout->addWidget(lblPrefix);
        compLayout->addWidget(spin);
        compLayout->addWidget(lblSuffix);
        compWidget->setLayout(compLayout);

        layout->addWidget(compWidget);
    }

    // Komment badge (ha van)
    if (!machine.comment.isEmpty()) {
        QLabel* lblComment = new QLabel(machine.comment);
        lblComment->setStyleSheet("QLabel { background-color: #f39c12; color: black; padding: 3px 8px; border-radius: 6px; }");
        layout->addWidget(lblComment);
    }

    layout->addStretch();
    badgeWidget->setLayout(layout);

    // 🔹 A StepId cellába tesszük be a composite widgetet
    vm.cells[CuttingInstructionTableColumns::StepId] =
        TableCellViewModel::fromWidget(badgeWidget, "Gép szeparátor sor");

    // A többi oszlop marad szeparátor
    for (int col = CuttingInstructionTableColumns::RodId;
         col <= CuttingInstructionTableColumns::Finalize; ++col) {
        vm.cells[col] = TableCellViewModel::fromText("—", "Separator", bg, fg, true);
    }

    return vm;
}


//     return vm;
// }

// inline TableRowViewModel generateMachineSeparator(const MachineHeader& machine) {
//     TableRowViewModel vm;
//     vm.rowId = QUuid::createUuid();

//     QColor bg = QColor("#B0C4DE"); // halvány kékesszürke háttér
//     QColor fg = Qt::black;

//     // Fő szöveg: gép neve
//     QString text = QString("=== %1 ===").arg(machine.machineName);

//     // Részletek: kerf, steller, kompenzáció
//     QStringList details;
//     details << QString("Kerf=%1 mm").arg(machine.kerf_mm, 0, 'f', 1);
//     if (machine.stellerMaxLength_mm.has_value())
//         details << QString("StellerMax=%1 mm").arg(machine.stellerMaxLength_mm.value(), 0, 'f', 1);
//     if (machine.stellerCompensation_mm.has_value())
//         details << QString("Comp=%1 mm").arg(machine.stellerCompensation_mm.value(), 0, 'f', 1);

//     if (!details.isEmpty())
//         text.append("   [" + details.join(" | ") + "]");

//     if (!machine.comment.isEmpty())
//         text.append(QString("   (%1)").arg(machine.comment));

//     // Csak az első cellát töltjük ki, a többiben vizuális szeparátor
//     vm.cells[CuttingInstructionTableColumns::StepId] =
//         TableCellViewModel::fromText(text, "Gép szeparátor sor", bg, fg);

//     for (int col = CuttingInstructionTableColumns::RodId;
//          col <= CuttingInstructionTableColumns::Finalize; ++col) {
//         vm.cells[col] = TableCellViewModel::fromText("—", "Separator", bg, fg, true);
//     }

//     return vm;
// }


/// Normál sor generálása
inline TableRowViewModel generate(const CutInstruction& ci,
                                  const QColor& baseColor,
                                  QObject* receiver = nullptr) {
    TableRowViewModel vm;
    vm.rowId = ci.rowId.isNull() ? QUuid::createUuid() : ci.rowId;

    const auto* mat = MaterialRegistry::instance().findById(ci.materialId);


    // 🎨 Alapszínek a csoport alapján
    //QColor baseColor = ColorLogicUtils::resolveBaseColor(mat);
    //QColor fgColor = baseColor.lightness() < 128 ? Qt::white : Qt::black;
    bool done = (ci.status == CutStatus::Done);

    // Ha Done → szürke háttér, sötét szöveg
    QColor rowBg = baseColor;
    QColor rowFg = (baseColor.lightness() < 128 ? Qt::white : Qt::black);

    if (done) {
        rowBg = QColor(220,220,220);   // világosszürke háttér
        rowFg = QColor(80,80,80);      // sötétszürke szöveg
    }

    vm.cells[CuttingInstructionTableColumns::StepId] =
        TableCellViewModel::fromText(QString::number(ci.globalStepId), "Lépés azonosító", rowBg, rowFg);

    // RodLabel: marad a CutPlan által generált label
    vm.cells[CuttingInstructionTableColumns::RodId] =
        TableCellViewModel::fromText(ci.rodId, "Rúd jel", rowBg, rowFg);

    // Barcode: ha van konkrét rod barcode, azt mutatjuk, material megy tooltipbe
    QString barcodeToShow = ci.barcode.isEmpty() ? "—" : ci.barcode;
    QString barcodeTooltip = QString("Rod barcode: %1\nMaterial: %2")
                                 .arg(ci.barcode.isEmpty() ? "—" : ci.barcode)
                                 .arg(mat ? mat->name : "Ismeretlen");

    // vm.cells[CuttingInstructionTableColumns::Barcode] =
    //     TableCellViewModel::fromText(barcodeToShow, barcodeTooltip, baseColor, fgColor);

    vm.cells[CuttingInstructionTableColumns::Material] =
        CellGenerators::materialCell(ci.materialId, ci.barcode, rowBg, rowFg);



    // QString cutText = QString("✂️ %1").arg(ci.cutSize_mm, 0, 'f', 1);
    // QString cutTooltip = "Vágandó hossz (mm)";

    QString cutText;
    QString cutTooltip;

    if (ci.isManualCut) {
        cutText = QString("📏 %1").arg(ci.cutSize_mm, 0, 'f', 1);
        cutTooltip = "Kézi jelölés – nincs steller kompenzáció";
    } else {
        cutText = QString("✂️ %1").arg(ci.effectiveCutSize_mm, 0, 'f', 1);
        cutTooltip = QString("Vágandó hossz kompenzációval (eredeti: %1 mm)")
                         .arg(ci.cutSize_mm, 0, 'f', 1);
    }

    // kiemelt háttér és betű
    QColor cutBg = baseColor.darker(120);
    QColor cutFg = Qt::white;
    QString cutStyle = "font-weight: bold; font-size: 14px; text-decoration: underline;";

    if (done) {
        cutBg = rowBg;   // szürke háttér
        cutFg = rowFg;   // sötét szöveg
        cutStyle.clear(); // ne legyen kiemelés
    }

    vm.cells[CuttingInstructionTableColumns::CutSize] =
        TableCellViewModel::fromStyledText(cutText,cutTooltip,cutBg,cutFg, cutStyle);

    vm.cells[CuttingInstructionTableColumns::LengthBefore] =
        TableCellViewModel::fromText(QString::number(ci.lengthBefore_mm, 'f', 1),
                                     "Vágás előtti hossz (mm)", rowBg, rowFg);


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

    // Ha Done → felülírjuk a kategória színeket is
    if (done) {
        bg = rowBg;
        fg = rowFg;
    }

    vm.cells[CuttingInstructionTableColumns::LengthAfter] =
        TableCellViewModel::fromText(afterText, afterTooltip, bg, fg);



    vm.cells[CuttingInstructionTableColumns::Status] =
        TableCellViewModel::fromText(
            CuttingStatusUtils::toText(ci.status),
            "Vágási státusz",
            rowBg,
            CuttingStatusUtils::toColor(ci.status)
            );

    // 🔑 Finalize cella logika
    if (ci.status == CutStatus::Done) {
        vm.cells[CuttingInstructionTableColumns::Finalize] =
            TableCellViewModel::fromText("✔", "Már végrehajtva",
                                         rowBg, rowFg, true);
    }
    else if (ci.status == CutStatus::InProgress) {
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
    else {
        vm.cells[CuttingInstructionTableColumns::Finalize] =
            TableCellViewModel::fromText("—", "Nem aktuális sor",
                                         rowBg, rowFg, true);
    }


    return vm;
}

} // namespace Cutting::ViewModel::RowGenerator
