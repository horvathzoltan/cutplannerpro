#include "storageaudittable_manager.h"
#include "common/logger.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
#include "common/tableutils/tableutils.h"
//#include "model/storageaudit/auditcontext_text.h"
#include "common/tableutils/tableutils_auditcells.h"
#include "model/storageaudit/storageauditentry.h"
#include "model/storageaudit/storageauditrow.h"

#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>

#include <model/registries/materialregistry.h>
#include "common/tableutils/auditcellformatter.h"

StorageAuditTableManager::StorageAuditTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), _parent(parent){
    _groupSync = std::make_unique<TableUtils::AuditGroupSynchronizer>(_table, _auditRowMap, _rows.rowIndexMap(), &_groupLabeler);

}

void StorageAuditTableManager::addRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    TableUtils::setMaterialNameCell(_table, rowIx, ColMaterial,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name()); // itt nincs egyedi entryId, csak materialId

    _rows.registerRow(rowIx, row.rowId); // opcionálisan extra: entry.storageId

    _table->setItem(rowIx, ColStorage, new QTableWidgetItem(row.storageName));

    QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";
    QString pickingQuantity = AuditCellFormatter::formatExpectedQuantity(row, groupLabel);
    QString missingQuantity = AuditCellFormatter::formatMissingQuantity(row);

    _table->setItem(rowIx, ColExpected, new QTableWidgetItem(pickingQuantity));
    _table->setItem(rowIx, ColMissing,  new QTableWidgetItem(missingQuantity));


    _table->setItem(rowIx, ColStatus,   new QTableWidgetItem(TableUtils::AuditCells::statusText(row)));
    //_table->setItem(rowIx, ColExpected,new QTableWidgetItem(pickingQuantity));

    _table->setItem(rowIx, ColBarcode, new QTableWidgetItem(row.barcode));

    QSpinBox* actualSpin = new QSpinBox();
    actualSpin->setRange(0, 9999);
    actualSpin->setValue(row.actualQuantity);
    actualSpin->setEnabled(true);
    actualSpin->setProperty(RowId_Key, row.rowId);
    _table->setCellWidget(rowIx, ColActual, actualSpin);

    //_table->setItem(rowIx, ColMissing,new QTableWidgetItem(missingQuantity));

    // QTableWidgetItem* statusItem = new QTableWidgetItem();
    // statusItem->setText(row.statusText());
    // _table->setItem(rowIx, ColStatus, statusItem);

    QObject::connect(actualSpin, &QSpinBox::valueChanged, this, [actualSpin, this]() {
        QUuid rowId = actualSpin->property(RowId_Key).toUuid();
        int actualQuantity = actualSpin->value();

        emit auditValueChanged(rowId, actualQuantity);
    });

    if (row.sourceType == AuditSourceType::Leftover) {
        auto container = new QWidget();
        auto layout = new QHBoxLayout(container); // vagy QHBoxLayout
        layout->setContentsMargins(0, 0, 0, 0);

        auto radioPresent = new QRadioButton("Van");
        auto radioMissing = new QRadioButton("Nincs");

        layout->addWidget(radioPresent);
        layout->addWidget(radioMissing);
        container->setLayout(layout);

        _table->setCellWidget(rowIx, ColActual, container);

        radioPresent->setProperty(RowId_Key, row.rowId);
        radioMissing->setProperty(RowId_Key, row.rowId);

        // 🔄 Állapot beállítása
        radioPresent->setChecked(row.actualQuantity > 0);
        radioMissing->setChecked(row.actualQuantity == 0);


        connect(radioPresent, &QRadioButton::toggled, this, [radioPresent, this]() {
            if (radioPresent->isChecked()) {
                QUuid rowId = radioPresent->property(RowId_Key).toUuid();
                emit leftoverPresenceChanged(rowId, AuditPresence::Present);
            }
        });

        connect(radioMissing, &QRadioButton::toggled, this, [radioMissing, this]() {
            if (radioMissing->isChecked()) {
                QUuid rowId = radioMissing->property(RowId_Key).toUuid();
                emit leftoverPresenceChanged(rowId, AuditPresence::Missing);
            }
        });
    }

    _auditRowMap[row.rowId] = row;                          // 🔹 Először frissítjük a mapet
    //_groupSync->syncRow(row);                               // 🔹 Ez már a mapből dolgozik → korrekt
    _groupSync->syncGroup(*row.context);                    // 🔹 Ez még workaroundos → később kiváltható
    StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);     // 🎨 Stílus
    StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row); // 🧠 Tooltip


    if (row.context) {
        zInfo(L("AuditContext [%1]: expected=%2, actual=%3, rows=%4")
                   .arg(row.materialId.toString())
                   .arg(row.context->group.totalExpected)
                   .arg(row.context->group.totalActual)
                   .arg(row.context->group.rowIds.size()));
    }

}

void StorageAuditTableManager::updateRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    if (row.context) {
        zInfo(L("🔄 Frissített AuditContext [%1]: expected=%2, actual=%3, rows=%4")
                   .arg(row.materialId.toString())
                   .arg(row.context->group.totalExpected)
                   .arg(row.context->group.totalActual)
                   .arg(row.context->group.rowIds.size()));
    }

    std::optional<int> rowIxOpt = _rows.rowOf(row.rowId);
    if (!rowIxOpt.has_value())
        return;
    int rowIx = rowIxOpt.value();

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    // 📛 Anyag név + szín
    TableUtils::setMaterialNameCell(_table, rowIx, ColMaterial,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name());

    // 🏷️ Tároló név
    auto* itemStorage = _table->item(rowIx, ColStorage);
    if (itemStorage)
        itemStorage->setText(row.storageName);

    auto* itemExpected = _table->item(rowIx, ColExpected);
    if (itemExpected) {
        QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";
        QString pickingQuantity = AuditCellFormatter::formatExpectedQuantity(row, groupLabel);
        itemExpected->setText(pickingQuantity);
    }

    auto* itemMissing = _table->item(rowIx, ColMissing);
    if (itemMissing){
        QString missingQuantity = row.isInOptimization?AuditCellFormatter::formatMissingQuantity(row):"-";
        itemMissing->setText(missingQuantity);
    }

    // 🎯 Elvárt mennyiség
    // auto* itemExpected = _table->item(rowIx, ColExpected);
    // if (itemExpected)
    //     itemExpected->setText(pickingQuantity);

    // ❌ Hiányzó mennyiség
    // auto* itemMissing = _table->item(rowIx, ColMissing);
    // if (itemMissing)
    //     itemMissing->setText(missingQuantity);


    // 📦 Tényleges mennyiség (SpinBox)
    auto* actualSpin = qobject_cast<QSpinBox*>(_table->cellWidget(rowIx, ColActual));
    if (actualSpin)
        actualSpin->setValue(row.actualQuantity);


    // ✅ Státusz
    auto* statusItem = _table->item(rowIx, ColStatus);
    if (statusItem) {
            statusItem->setText(TableUtils::AuditCells::statusText(row));
    }

    auto container = qobject_cast<QWidget*>(_table->cellWidget(rowIx, ColActual));
    if (container) {
        auto radios = container->findChildren<QRadioButton*>();
        for (auto* radio : radios) {
            if (radio->text() == "Van")
                radio->setChecked(row.actualQuantity > 0);
            else if (radio->text() == "Nincs")
                radio->setChecked(row.actualQuantity == 0);
        }
    }

    // 🎨 Stílus újraalkalmazás
    _auditRowMap[row.rowId] = row;                          // 🔹 Először frissítjük a mapet
    //_groupSync->syncRow(row);                               // 🔹 Ez már a mapből dolgozik → korrekt
    _groupSync->syncGroup(*row.context);                           // 🔹 Ez még workaroundos → később kiváltható
    StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);     // 🎨 Stílus
    StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row); // 🧠 Tooltip

}

void StorageAuditTableManager::clearTable() {
    _table->clearContents();
    _table->setRowCount(0);
    _rows.clear();
    _groupLabeler.clear();
}

// void StorageAuditTableManager::applyGroupContextToRows(const StorageAuditRow& row) {
//     if (!row.context || row.context->group.rowIds.size() <= 1)
//         return;

//     for (const QUuid& groupRowId : row.context->group.rowIds) {
//         std::optional<int> rowIxOpt = _rows.rowOf(groupRowId);
//         if (!rowIxOpt.has_value())
//             continue;

//         int rowIx = rowIxOpt.value();
//         const StorageAuditRow& groupRow = /* valahonnan lekérve, pl. auditRowMap[groupRowId] */ row; // ha nincs map, akkor workaround kell

//         QString expectedText = AuditCellFormatter::formatExpectedQuantity(row); // mindig a context alapján
//         QString missingText  = AuditCellFormatter::formatMissingQuantity(row);
//         QString statusText   = TableUtils::AuditCells::statusText(row);

//         if (auto* itemExpected = _table->item(rowIx, ColExpected))
//             itemExpected->setText(expectedText);

//         if (auto* itemMissing = _table->item(rowIx, ColMissing))
//             itemMissing->setText(missingText);

//         if (auto* itemStatus = _table->item(rowIx, ColStatus))
//             itemStatus->setText(statusText);
//     }
// }
