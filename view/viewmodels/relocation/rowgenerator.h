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

    // Quantity cell text and color logic
    QString qtyText;
    QColor qtyColor = fgColor; // alapértelmezett: normál szöveg színe

    if (instr.isSatisfied) {
        qtyText = QStringLiteral("✔ Megvan");
        qtyColor = QColor("#228B22"); // zöld
    } else if (instr.plannedQuantity == 0) {
        // nincs szükség mozgatásra — semleges státusz, ne pirosítsuk
        qtyText = QStringLiteral("✔ Megvan");//QStringLiteral("");
        qtyColor = QColor("#228B22"); // zöld //QColor("#666666"); // semleges/sötétszürke
    } else {
        // van igény (>0) — mutassuk a számot, és piros csak ha kifejezetten hiány van
        qtyText = QString::number(instr.plannedQuantity);

        // meghatározó feltételek a piros jelzéshez:
        // - nincs elég available a forrásokban, vagy
        // - a dialog/gazdálkodás szerint maradék > 0 és nincs cél fedezet
        const bool insufficientSource = instr.availableQuantity() < instr.plannedRemaining();
        const bool insufficientTarget = instr.totalPlaced() < instr.plannedRemaining();

        if (insufficientSource || insufficientTarget) {
            qtyColor = QColor("#B22222"); // piros: probléma / hiány
        } else {
            qtyColor = fgColor; // nincs probléma, alap szín
        }
    }

    QString qtyTooltip;
    if (instr.plannedQuantity == 0 && !instr.isSatisfied) {
        qtyTooltip = QStringLiteral("Nincs szükség áthelyezésre.");
    } else {
        qtyTooltip = QString("Terv: %1; Elérhető: %2; Dialógusban: %3")
                         .arg(instr.plannedQuantity)
                         .arg(instr.availableQuantity())
                         .arg(instr.sourcesTotalMovedQuantity());
    }

    vm.cells[RelocationPlanTableColumns::Quantity] =
        TableCellViewModel::fromText(qtyText, qtyTooltip, baseColor, qtyColor);

    // a helper feltételek
    const bool needsMove = instr.plannedRemaining() > 0;
    //const bool editableAllowed = needsMove && !instr.isAlreadyFinalized();
    const bool editableAllowed = needsMove && !instr.isAlreadyFinalized() && !instr.isLeftover();

    // Forrás cella
    if (instr.sourceType == AuditSourceType::Stock) {
        QStringList sourceParts;
        for (const auto& src : instr.sources) {
            sourceParts << QString("%1 (%2/%3)")
            .arg(src.locationName)
                .arg(src.moved)
                .arg(src.available);
        }
        QString sourceText = sourceParts.isEmpty() ? QStringLiteral("—") : sourceParts.join(", ");

        if (editableAllowed) {
            vm.cells[RelocationPlanTableColumns::Source] =
                CellGenerator::createEditableCell(vm.rowId,
                                   sourceText,
                                   QString("Források szerkesztése — jelenleg: %1").arg(sourceText),
                                   receiver,
                                   "source");
        } else {
            // read-only megjelenítés, tooltip magyarázattal
            QString tip = instr.isAlreadyFinalized()
                              ? QStringLiteral("A sor véglegesítve. Szerkesztés nem engedélyezett.")
                              : QStringLiteral("Nincs szükség áthelyezésre; szerkesztés nem szükséges.");
            vm.cells[RelocationPlanTableColumns::Source] =
                TableCellViewModel::fromText(sourceText, tip, baseColor, fgColor, true);
        }

        // Cél cella
        QStringList targetParts;
        for (const auto& tgt : instr.targets) {
            targetParts << QString("%1 (%2)")
            .arg(tgt.locationName)
                .arg(tgt.placed);
        }
        QString targetText = targetParts.isEmpty() ? QStringLiteral("—") : targetParts.join(", ");

        if (editableAllowed) {
            vm.cells[RelocationPlanTableColumns::Target] =
                CellGenerator::createEditableCell(vm.rowId,
                                   targetText,
                                   QString("Célok szerkesztése — jelenleg: %1").arg(targetText),
                                   receiver,
                                   "target");
        } else {
            QString tip = instr.isAlreadyFinalized()
            ? QStringLiteral("A sor véglegesítve. Szerkesztés nem engedélyezett.")
            : QStringLiteral("Nincs szükség áthelyezésre; szerkesztés nem szükséges.");
            vm.cells[RelocationPlanTableColumns::Target] =
                TableCellViewModel::fromText(targetText, tip, baseColor, fgColor, true);
        }
    } else {
        // hulló / egyéb forrás típusok maradnak változatlanul, gomb nélkül
        QString sourceText = instr.sources.isEmpty() ? QStringLiteral("—") : instr.sources.first().locationName;
        vm.cells[RelocationPlanTableColumns::Source] =
            TableCellViewModel::fromText(sourceText,
                                         QString("Hulló forrás: %1").arg(sourceText),
                                         baseColor, fgColor, true);
        vm.cells[RelocationPlanTableColumns::Target] =
            TableCellViewModel::fromText(QStringLiteral("—"),
                                         QStringLiteral("Hullónál nincs cél"),
                                         baseColor, fgColor, true);
    }

    // if (instr.sourceType == AuditSourceType::Stock) {

    //     // forrás/cél cella eleje
    //     QStringList sourceParts;
    //     for (const auto& src : instr.sources) {
    //         sourceParts << QString("%1 (%2/%3)")
    //         .arg(src.locationName)
    //             .arg(src.moved)
    //             .arg(src.available);
    //     }
    //     QString sourceText = sourceParts.isEmpty() ? QStringLiteral("—") : sourceParts.join(", ");
    //     vm.cells[RelocationPlanTableColumns::Source] =
    //         CellGenerator::createEditableCell(vm.rowId,
    //                                           sourceText,
    //                                           QString("Forrás tárhelyek: %1").arg(sourceText),
    //                                           receiver,
    //                                           "source");

    //     QStringList targetParts;
    //     for (const auto& tgt : instr.targets) {
    //         targetParts << QString("%1 (%2)")
    //         .arg(tgt.locationName)
    //             .arg(tgt.placed);
    //     }
    //     QString targetText = targetParts.isEmpty() ? QStringLiteral("—") : targetParts.join(", ");
    //     vm.cells[RelocationPlanTableColumns::Target] =
    //         CellGenerator::createEditableCell(vm.rowId,
    //                                           targetText,
    //                                           QString("Cél tárhelyek: %1").arg(targetText),
    //                                           receiver,
    //                                           "target");

    //     //
    // } else {
    //     QString sourceText = instr.sources.isEmpty() ? QStringLiteral("—") : instr.sources.first().locationName;
    //     vm.cells[RelocationPlanTableColumns::Source] =
    //         TableCellViewModel::fromText(sourceText,
    //                               QString("Hulló forrás: %1").arg(sourceText),
    //                               baseColor, fgColor);

    //     vm.cells[RelocationPlanTableColumns::Target] =
    //         TableCellViewModel::fromText(QStringLiteral("—"),
    //                               QStringLiteral("Hullónál nincs cél"),
    //                               baseColor, fgColor);
    // }

    QString typeText = (instr.sourceType == AuditSourceType::Stock)
                           ? QStringLiteral("📦 Stock")
                           : QStringLiteral("♻️ Hulló");
    vm.cells[RelocationPlanTableColumns::Type] =
        TableCellViewModel::fromText(typeText,
                              QString("Forrás típusa: %1").arg(typeText),
                              baseColor, fgColor);

    // Finalize gomb cella (frissített logika)
    // döntő feltételek előkészítése
    //const bool needsMove = instr.plannedRemaining() > 0;
    //const bool editableAllowed = needsMove && !instr.isAlreadyFinalized() && !instr.isLeftover();

    if (!needsMove) {
        // Nincs teendő: ne mutassuk a finalize gombot — egyszerű, olvasható cella
        vm.cells[RelocationPlanTableColumns::Finalize] =
            TableCellViewModel::fromText(QStringLiteral("—"),
                                         QStringLiteral("Nincs szükség finalizálásra."),
                                         baseColor, fgColor, true);
    } else if (instr.isAlreadyFinalized()) {
        // Már véglegesítve
        vm.cells[RelocationPlanTableColumns::Finalize] =
            TableCellViewModel::fromText(QStringLiteral("✔"),
                                         QStringLiteral("A sor már véglegesítve lett."),
                                         baseColor, QColor("#666666"), true);
    } else if (instr.isLeftover()) {
        // Hulló: soha nem finalizáljuk
        vm.cells[RelocationPlanTableColumns::Finalize] =
            TableCellViewModel::fromText(QStringLiteral("—"),
                                         QStringLiteral("Hulladékot nem finalizálunk."),
                                         baseColor, QColor("#999999"), true);
    } else {
        // Van teendő és nem finalizált: építsük fel intelligensen a gombot
        QPushButton* btn = new QPushButton("Finalize");
        btn->setCursor(Qt::PointingHandCursor);

        // Dinamikus tooltip összeállítása
        QString tip;
        if (!instr.hasTarget()) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: #eee; color: #999;");
            tip = QStringLiteral("Nincs cél. Hozz létre célt vagy válassz meglévőt a sor szerkesztésével.");
        } else if (!instr.isReadyToFinalize()) {
            btn->setEnabled(false);
            btn->setStyleSheet("background-color: #eee; color: #999;");
            // részletes ok: forrás hiány vagy cél nem fedezett
            if (instr.availableQuantity() < instr.plannedRemaining()) {
                tip = QStringLiteral("A forrás nem tartalmaz elegendő mennyiséget a véglegesítéshez.");
            } else {
                tip = QStringLiteral("A célok nem fedezik a szükséges mennyiséget a véglegesítéshez.");
            }
        } else {
            btn->setEnabled(true);
            btn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
            tip = QStringLiteral("A sor véglegesíthető. Kattintás végrehajtja az áthelyezést.");
        }

        // Tooltip beállítása mind a widgetre, mind a VM-re (populateRow már widget->setToolTip-ot használ)
        btn->setToolTip(tip);

        QObject::connect(btn, &QPushButton::clicked, receiver, [receiver, rowId = instr.rowId]() {
            QMetaObject::invokeMethod(receiver, "finalizeRow", Qt::QueuedConnection,
                                      Q_ARG(QUuid, rowId));
        });

        vm.cells[RelocationPlanTableColumns::Finalize] =
            TableCellViewModel::fromWidget(btn, tip);
    }


    // if (instr.sourceType == AuditSourceType::Stock) {
    //     QPushButton* btn = new QPushButton("Finalize");
    //     btn->setCursor(Qt::PointingHandCursor);

    //     qInfo() << "Row" << instr.rowId
    //             << "isFinalized=" << instr.isAlreadyFinalized()
    //             << "isLeftover=" << instr.isLeftover()
    //             << "hasTarget=" << instr.hasTarget()
    //             << "targets.size=" << instr.targets.size()
    //             << "planned=" << instr.plannedQuantity
    //             << "available=" << instr.availableQuantity();

    //     if (instr.isAlreadyFinalized()) {
    //         btn->setText("✔");
    //         btn->setEnabled(false);
    //         btn->setStyleSheet("background-color: #ccc; color: #666;");
    //         btn->setToolTip(QStringLiteral("A sor már véglegesítve lett."));
    //     } else if (instr.isLeftover()) {
    //         btn->setEnabled(false);
    //         btn->setStyleSheet("background-color: #eee; color: #999;");
    //         btn->setToolTip(QStringLiteral("Hulladékot nem finalizálunk."));
    //     } else if (!instr.hasTarget()) {
    //         btn->setEnabled(false);
    //         btn->setStyleSheet("background-color: #eee; color: #999;");
    //         btn->setToolTip(QStringLiteral("Nincs cél. Hozz létre célt vagy válassz meglévőt a sor szerkesztésével."));
    //     } else if (!instr.isReadyToFinalize()) {
    //         btn->setEnabled(false);
    //         btn->setStyleSheet("background-color: #eee; color: #999;");
    //         btn->setToolTip(QStringLiteral("A forrás nem tartalmaz elegendő mennyiséget a véglegesítéshez."));
    //     } else {
    //         btn->setEnabled(true);
    //         btn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    //         btn->setToolTip(QStringLiteral("A sor véglegesíthető."));
    //     }

    //     QObject::connect(btn, &QPushButton::clicked, receiver, [receiver, rowId = instr.rowId]() {
    //         QMetaObject::invokeMethod(receiver, "finalizeRow", Qt::QueuedConnection,
    //                                   Q_ARG(QUuid, rowId));
    //     });

    //     vm.cells[RelocationPlanTableColumns::Finalize] =
    //         TableCellViewModel::fromWidget(btn);
    // } else {
    //     // Hullónál ne legyen Finalize gomb — üres, nem interaktív cella
    //     vm.cells[RelocationPlanTableColumns::Finalize] =
    //         TableCellViewModel::fromText("-",
    //                               QStringLiteral("Nincs finalize hullónál"),
    //                               baseColor, fgColor);
    // }


    return vm;
}



} // namespace RelocationRowViewModelGenerator
