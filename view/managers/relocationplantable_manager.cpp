#include "relocationplantable_manager.h"
#include "view/viewmodels/relocation/rowgenerator.h"
//#include "view/columnindexes/relocationplantable_columns.h"
#include "common/logger.h"

#include "view/dialog/relocation/relocationquantitydialog.h"
#include "view/tablehelpers/relocationquantityhelpers.h"

#include "view/tablehelpers/tablerowpopulator.h"

#include <QTableWidgetItem>
#include <QHeaderView>
#include <QBrush>
#include <QColor>

#include <model/registries/materialregistry.h>

#include <service/stockmovementservice.h>

bool RelocationPlanTableManager::_isVerbose = false;

/**
 * @brief Konstruktor – inicializálja a táblát és beállítja az oszlopfejléceket.
 */
RelocationPlanTableManager::RelocationPlanTableManager(QTableWidget* table,
                                                       CuttingPresenter* presenter,
                                                       QWidget* parent = nullptr)
    : QObject(parent), _table(table), _parent(parent), _presenter(presenter)
{
   // a tábla oszlopai a designerben kerültek definiálásra
}

/**
 * @brief Új sor beszúrása a relokációs terv táblába a generator segítségével.
 *
 * - Meghívja a RelocationRowViewModelGenerator::generate()-t
 * - A kapott TableRowViewModel celláit beírja a QTableWidget-be
 * - A rowId-t eltárolja a belső map-ekben
 */
void RelocationPlanTableManager::addRow(const RelocationInstruction& instr) {
    if (!_table) return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    // 🔍 Anyag lekérése az azonosító alapján
    const MaterialMaster* mat = nullptr;
    // if(instr.sourceType == AuditSourceType::Stock){
    //      mat = MaterialRegistry::instance().findByBarcode(instr.barcode);
    // } else {
    //     std::optional<LeftoverStockEntry> a =
    //         LeftoverStockRegistry::instance().findByBarcode(instr.barcode);
    //     if(a.has_value())
    //         mat = MaterialRegistry::instance().findById(a->materialId);
    // }
    mat = MaterialRegistry::instance().findById(instr.materialId);
    if (!mat)
        return;

    // ViewModel generálása és cellák kirenderelése
    TableRowViewModel vm = Relocation::ViewModel::RowGenerator::generate(instr, *mat, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // rowId mentése
    //_planRowMap[vm.rowId] = instr;
    _planRowMap.insert(vm.rowId, instr);
    _rowIndexMap[vm.rowId] = rowIx;


    if (_isVerbose) {
        zInfo(L("RelocationPlan row added via generator: %1 | %2")
                  .arg(instr.materialName)
                  .arg(instr.barcode));
    }
}

/**
 * @brief Meglévő sor frissítése rowId alapján.
 *
 * - Újra legenerálja a ViewModel-t a frissített RelocationInstruction-ből
 * - Frissíti a cellák tartalmát a táblában
 */
void RelocationPlanTableManager::updateRow(const QUuid& rowId, const RelocationInstruction& instr) {
    if (!_table) return;
    if (!_rowIndexMap.contains(rowId)) return;

    int rowIx = _rowIndexMap.value(rowId);
    //_planRowMap[rowId] = instr;
    // updateRow-ban:
    _planRowMap.insert(rowId, instr);

    // 🔍 Anyag lekérése az azonosító alapján
    const MaterialMaster* mat = nullptr;
    // if(instr.sourceType == AuditSourceType::Stock){
    //     mat = MaterialRegistry::instance().findByBarcode(instr.barcode);
    // } else {
    //     std::optional<LeftoverStockEntry> a =
    //         LeftoverStockRegistry::instance().findByBarcode(instr.barcode);
    //     if(a.has_value())
    //         mat = MaterialRegistry::instance().findById(a->materialId);
    // }
    mat = MaterialRegistry::instance().findById(instr.materialId);
    if (!mat)
        return;

    // ViewModel generálása és cellák kirenderelése
    TableRowViewModel vm = Relocation::ViewModel::RowGenerator::generate(instr, *mat, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    if (_isVerbose) {
        zInfo(L("RelocationPlan row updated via generator: %1 | %2")
                  .arg(instr.materialName)
                  .arg(instr.barcode));
    }
}

/**
 * @brief Teljes tábla törlése.
 */
void RelocationPlanTableManager::clearTable() {
    if (!_table) return;

    _table->clearContents();
    _table->setRowCount(0);
    _planRowMap.clear();
    _rowIndexMap.clear();

    if (_isVerbose) {
        zInfo(L("RelocationPlan table cleared"));
    }
}

void RelocationPlanTableManager::editRow(const QUuid& rowId, const QString& mode) {
    auto it = _planRowMap.find(rowId);
    if (it == _planRowMap.end())
        return;

    RelocationInstruction& instruction = it.value();

    // közvetlenül az instruction referencia után
    zInfo("---- Instruction full dump ----");
    zInfo(QString("materialId=%1 materialName=%2 plannedQuantity=%3 totalMovedQuantity=%4 isFinalized=%5 isSummary=%6")
              .arg(instruction.materialId.toString())
              .arg(instruction.materialName)
              .arg(instruction.plannedQuantity)
              .arg(instruction.sourcesTotalMovedQuantity())
              .arg(instruction.isAlreadyFinalized())
              .arg(instruction.isSummary));
    zInfo(QString("isSatisfied=%1 barcode=%2 sourceType=%3")
              .arg(instruction.isSatisfied)
              .arg(instruction.barcode)
              .arg(static_cast<int>(instruction.sourceType)));

    zInfo(QString("sources.count=%1").arg(instruction.sources.size()));
    for (const auto& s : instruction.sources) {
        zInfo(QString("SRC: entryId=%1, locationId=%2, locationName=%3, available=%4, moved=%5")
                  .arg(s.entryId.isNull() ? QString("{NULL}") : s.entryId.toString())
                  .arg(s.locationId.isNull() ? QString("{NULL}") : s.locationId.toString())
                  .arg(s.locationName)
                  .arg(s.available)
                  .arg(s.moved));
    }

    zInfo(QString("targets.count=%1").arg(instruction.targets.size()));
    for (const auto& t : instruction.targets) {
        zInfo(QString("TGT: locationId=%1, locationName=%2, placed=%3")
                  .arg(t.locationId.isNull() ? QString("{NULL}") : t.locationId.toString())
                  .arg(t.locationName)
                  .arg(t.placed));
    }

    zInfo(QString("summaryText=%1 totalRemaining=%2 auditedRemaining=%3 movedQty=%4 uncoveredQty=%5 coveredQty=%6 usedFromRemaining=%7")
              .arg(instruction.summaryText)
              .arg(instruction.totalRemaining)
              .arg(instruction.auditedRemaining)
              .arg(instruction.movedQty)
              .arg(instruction.uncoveredQty)
              .arg(instruction.coveredQty)
              .arg(instruction.usedFromRemaining));
    zInfo("---- End of instruction dump ----");




    // 🔹 Dialógus előkészítése
    RelocationQuantityDialog dlg(_parent);

    if (mode == "source") {
        dlg.setMode(QuantityDialogMode::Source);
        dlg.setWindowTitle(tr("Forrás tárhelyek szerkesztése"));

        auto rows = RelocationQuantityHelpers::generateSourceRows(instruction);

        zInfo("---- Before dlg.setRows (generated source rows) ----");
        for (const auto& r : rows) {
            zInfo(QString("GEN r.entryId=%1, storageId=%2, selected=%3, available=%4, name=%5")
                      .arg(r.entryId.toString())
                      .arg(r.storageId.toString())
                      .arg(r.selected)
                      .arg(r.available)
                      .arg(r.storageName));
        }
        zInfo("---- End of generated rows dump ----");

        dlg.setRows(rows, instruction.plannedQuantity, -1);
        zInfo(L("generateSourceRows: %1 sor").arg(rows.size()));
    }
    else if (mode == "target") {
        dlg.setMode(QuantityDialogMode::Target);
        dlg.setWindowTitle(tr("Cél tárhelyek szerkesztése"));

        auto rows = RelocationQuantityHelpers::generateTargetRows(instruction);

        zInfo("---- Before dlg.setRows (generated target rows) ----");
        for (const auto& r : rows) {
            zInfo(QString("GEN tgt.storageId=%1, storageName=%2, selected=%3, current=%4")
                      .arg(r.storageId.toString())
                      .arg(r.storageName)
                      .arg(r.selected)
                      .arg(r.current));
        }
        zInfo("---- End of generated target rows dump ----");

        int totalMoved = 0;
        for (const auto& src : instruction.sources)
            totalMoved += src.moved;

        dlg.setRows(rows, instruction.plannedQuantity, totalMoved);
        zInfo(L("generateTargetRows: %1 sor").arg(rows.size()));
    }
    else {
        return; // nincs combined mód, korábban fallback volt, de te törölted
    }

    // meghívjuk a dialógust
    int a = dlg.exec();

    if (a == QDialog::Accepted) {
        QVector<RelocationQuantityRow> result = dlg.getRows();

        if (mode == "source") {
            RelocationQuantityHelpers::applySourceRows(instruction, result);

            zInfo("---- After applySourceRows ----");
            for (const auto& s : instruction.sources) {
                zInfo(QString("src.entryId=%1, locId=%2, moved=%3, available=%4, name=%5")
                          .arg(s.entryId.toString())
                          .arg(s.locationId.toString())
                          .arg(s.moved)
                          .arg(s.available)
                          .arg(s.locationName));
            }
            zInfo("---- End of applySourceRows dump ----");

        } else if (mode == "target") {
            RelocationQuantityHelpers::applyTargetRows(instruction, result);

            zInfo("---- After applyTargetRows ----");
            for (const auto& t : instruction.targets) {
                zInfo(QString("tgt.locId=%1, locName=%2, placed=%3")
                          .arg(t.locationId.toString())
                          .arg(t.locationName)
                          .arg(t.placed));
            }
            zInfo("---- End of applyTargetRows dump ----");

        }

        updateRow(rowId, instruction); // frissítés a táblában
    }
}

void RelocationPlanTableManager::finalizeRow(const QUuid& rowId) {
    // 🔹 Keressük meg a sort
    auto it = _planRowMap.find(rowId);
    if (it == _planRowMap.end()) {
        zWarning(QStringLiteral("finalizeRow: row not found %1").arg(rowId.toString()));
        return;
    }

    RelocationInstruction& instr = it.value();

    // Minden targetnek legyen storage
    for (const auto& tgt : instr.targets) {
        if (!tgt.locationId.isNull()) continue;
        zWarning(QStringLiteral("❌ finalizeRow: target locationId is NULL for material %1").arg(instr.materialId.toString()));
        return;
    }

    // Ne fusson, ha már finalizálták
    if (instr.isAlreadyFinalized()) {
        zInfo(QStringLiteral("finalizeRow: already finalized for row %1").arg(rowId.toString()));
        return;
    }

    // Strict előfeltétel (végső gate)
    if (!instr.isReadyToFinalize_Strict()) {
        zInfo(QStringLiteral("finalizeRow: strict preconditions not met for row %1; plannedRemaining=%2 available=%3 totalPlaced=%4")
                  .arg(rowId.toString())
                  .arg(instr.plannedRemaining())
                  .arg(instr.availableQuantity())
                  .arg(instr.totalPlaced()));
        return;
    }

    const int toExecute = instr.plannedRemaining();
    if (toExecute <= 0) {
        zWarning(QStringLiteral("finalizeRow: nothing to execute for row %1").arg(rowId.toString()));
        return;
    }

    // Debug dump
    zInfo(QStringLiteral("---- Finalize instruction dump for row %1 ----").arg(rowId.toString()));
    for (const auto& src : instr.sources) {
        zInfo(QStringLiteral("Finalize source: entryId=%1, moved=%2, available=%3, locationId=%4")
                  .arg(src.entryId.toString())
                  .arg(src.moved)
                  .arg(src.available)
                  .arg(src.locationId.toString()));
    }
    for (const auto& tgt : instr.targets) {
        zInfo(QStringLiteral("Finalize target: locationId=%1, placed=%2")
                  .arg(tgt.locationId.toString())
                  .arg(tgt.placed));
    }
    zInfo(QStringLiteral("---- End of finalize dump for row %1 ----").arg(rowId.toString()));

    // Szolgáltatás meghívása, opcionális requestId mint idempotencia token
    //const QString requestId = QString::number(QDateTime::currentMSecsSinceEpoch());
    StockMovementService svc(_presenter);
    if (svc.finalizeRelocation(instr)) {
        // Sikeres perzisztálás: frissítjük a UI-t a szolgáltatás által esetlegesen módosított instr alapján
        updateRow(rowId, instr);
        zInfo(QStringLiteral("finalizeRow: success for row %1 material=%2 totalMovedQuantity=%3")
                  .arg(rowId.toString()).arg(instr.materialId.toString()).arg(instr.sourcesTotalMovedQuantity()));
    } else {
        // Részletes hibalog: mi próbálkozott, mi volt a várt állapot, mi hiányzott
        zError(QStringLiteral("finalizeRow: service failed to finalize row %1 material=%2 plannedRemaining=%3 available=%4 totalPlaced=%5")
                   .arg(rowId.toString())
                   .arg(instr.materialId.toString())
                   .arg(instr.plannedRemaining())
                   .arg(instr.availableQuantity())
                   .arg(instr.totalPlaced()));

        // Opcionális: presenternek jelezve a sor újratöltését, hogy az UI a perzisztált állapotot mutassa
        /*f (_presenter) {
            QMetaObject::invokeMethod(_presenter, "refreshRow", Qt::QueuedConnection, Q_ARG(QUuid, rowId));
        }*/
    }


    // Sikeres perzisztálás után frissítjük a UI-t a szolgáltatás által esetlegesen módosított instr alapján
    updateRow(rowId, instr);
    zInfo(QStringLiteral("finalizeRow: success for row %1 executed=%2").arg(rowId.toString()).arg(toExecute));
}

void RelocationPlanTableManager::updateSummaryRow(const RelocationInstruction& summaryInstr) {
    for (const auto& rowId : allRowIds()) {
        const RelocationInstruction& existing = getInstruction(rowId);

        if (existing.isSummary && existing.materialId == summaryInstr.materialId) {
            overwriteRow(rowId, summaryInstr);
            return;
        }
    }
    // Ha nem találtunk meglévőt, új sor beszúrása
    addRow(summaryInstr);
}


// relocationplantable_manager.cpp
const RelocationInstruction& RelocationPlanTableManager::getInstruction(const QUuid& rowId) const {
    auto it = _planRowMap.find(rowId);
    if (it == _planRowMap.end()) {
        throw std::runtime_error(("RowId not found: " + rowId.toString()).toStdString());
    }
    return it.value();
}

RelocationInstruction& RelocationPlanTableManager::getInstruction(const QUuid& rowId) {
    auto it = _planRowMap.find(rowId);
    if (it == _planRowMap.end()) {
        throw std::runtime_error(("RowId not found: " + rowId.toString()).toStdString());
    }
    return it.value();
}

void RelocationPlanTableManager::overwriteRow(const QUuid& rowId, const RelocationInstruction& instr) {
    if (!_rowIndexMap.contains(rowId)) return;

    int rowIx = _rowIndexMap.value(rowId);
    auto it = _planRowMap.find(rowId);
    if (it != _planRowMap.end()) {
        it.value() = instr; // közvetlen felülírás másolással
    } else {
        _planRowMap.insert(rowId, instr);
    }
    const MaterialMaster* mat = MaterialRegistry::instance().findById(instr.materialId);
    if (!mat) return;

    TableRowViewModel vm = Relocation::ViewModel::RowGenerator::generate(instr, *mat, this);
    TableRowPopulator::populateRow(_table, rowIx, vm);

    if (_isVerbose) {
        zInfo(L("RelocationPlan row overwritten: %1 | %2")
                  .arg(instr.materialName)
                  .arg(instr.barcode));
    }
}

// void RelocationPlanTableManager::finalizeRow(const QUuid& rowId) {

//     // 🔹 Keressük meg a sort
//     auto it = _planRowMap.find(rowId);
//     if (it == _planRowMap.end())
//         return;

//     RelocationInstruction& instr = it.value();

//     for (const auto& tgt : instr.targets) {
//         if (!tgt.locationId.isNull()) continue;
//         qWarning() << "❌ finalizeRow: target locationId is NULL for material" << instr.materialId;
//         return;
//     }

//     // 🔹 Csak akkor futtatjuk, ha tényleg finalizálható
//     if (!instr.isReadyToFinalize() || instr.isAlreadyFinalized())
//         return;

//     zInfo("---- Finalize instruction dump ----");
//     for (const auto& src : instr.sources) {
//         zInfo(QString("Finalize source: entryId=%1, moved=%2, locationId=%3")
//                   .arg(src.entryId.toString())
//                   .arg(src.moved)
//                   .arg(src.locationId.toString()));
//     }

//     for (const auto& tgt : instr.targets) {
//         zInfo(QString("Finalize target: locationId=%1, placed=%2")
//                   .arg(tgt.locationId.toString())
//                   .arg(tgt.placed));
//     }
//     zInfo("---- End of finalize dump ----");

//     // 🔹 Service példány presenter-rel
//     StockMovementService svc(_presenter);
//     if (svc.finalizeRelocation(instr)) {
//         updateRow(rowId, instr); // UI frissítés
//     }
// }


