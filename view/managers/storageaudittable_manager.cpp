#include "storageaudittable_manager.h"

#include "common/logger.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
//#include "common/tableutils/tableutils.h"
//#include "common/tableutils/tableutils_auditcells.h"
#include "common/tableutils/auditcellformatter.h"

#include "model/storageaudit/storageauditentry.h"
#include "model/storageaudit/storageauditrow.h"
#include "model/registries/materialregistry.h"

#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include "view/cellgenerators/auditrowviewmodelgenerator.h"
//#include "view/columnidexes/audittablecolumns.h"
#include "view/tablehelpers/tablerowpopulator.h"

StorageAuditTableManager::StorageAuditTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), _parent(parent)
{
    // 🔗 Csoport szinkronizáló inicializálása – a sorok közötti logikai kapcsolatért felel
    _groupSync = std::make_unique<TableUtils::AuditGroupSynchronizer>(
        _table,
        _auditRowMap,
        _rows.rowIndexMap(),
        &_groupLabeler,
        this // szükséges, hogy a signalokat vissza tudjuk vezetni
    );
}

void StorageAuditTableManager::addRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    // 🔍 Anyag lekérése az azonosító alapján
    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    // ➕ Új sor beszúrása
    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    // 🗂️ AuditRow mentése a belső map-be
    _auditRowMap[row.rowId] = row;

    // 🧱 Widgetek létrehozása (pl. QSpinBox, QRadioButton)
    //createAuditRowWidgets(row, rowIx);

    // 🏷️ Csoportcímke lekérése
    QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";

    // 🧱 ViewModel generálása
    TableRowViewModel vm = AuditRowViewModelGenerator::generate(row, mat, groupLabel, this);

    // 🧩 Megjelenítés
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // 🧩 Cellák feltöltése tartalommal
    //populateAuditRowContent(row, rowIx, groupLabel);

    // 🧭 Sorregisztráció – segít a sorok azonosításában
    _rows.registerRow(rowIx, row.rowId);

    // 🔁 Csoport szinkronizálása – ha van AuditContext
    if (row.context)
        _groupSync->syncGroup(*row.context, row.rowId);

    // 🎨 Stílus és tooltip alkalmazása
    //StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);
    //StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row);

    // 🧠 Naplózás – csak ha van AuditContext
    if (row.context) {
        zInfo(L("AuditContext [%1]: expected=%2, actual=%3, rows=%4")
            .arg(row.materialId.toString())
            .arg(row.context->totalExpected)
            .arg(row.context->totalActual)
            .arg(row.context->group.size()));
    }
}

void StorageAuditTableManager::updateRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    // 🧠 Naplózás – frissített AuditContext
    if (row.context) {
        zInfo(L("🔄 Frissített AuditContext [%1]: expected=%2, actual=%3, rows=%4")
            .arg(row.materialId.toString())
            .arg(row.context->totalExpected)
            .arg(row.context->totalActual)
            .arg(row.context->group.size()));
    }

    // 🔍 Sorindex lekérése a rowId alapján
    std::optional<int> rowIxOpt = _rows.rowOf(row.rowId);
    if (!rowIxOpt.has_value())
        return;

    int rowIx = rowIxOpt.value();

    // 🗂️ Frissített AuditRow mentése
    _auditRowMap[row.rowId] = row;

    // 🏷️ Csoportcímke újra lekérése
    QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";

    // 🧩 Cellák újratöltése
    //populateAuditRowContent(row, rowIx, groupLabel);

    // 🧱 ViewModel generálása
    TableRowViewModel vm = AuditRowViewModelGenerator::generate(row, mat, groupLabel, this);

    // 🧩 Megjelenítés
    TableRowPopulator::populateRow(_table, rowIx, vm);

    // 🔁 Csoport újraszinkronizálása
    if (row.context)
        _groupSync->syncGroup(*row.context, row.rowId);

    // 🎨 Stílus és tooltip újraalkalmazása
    //StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);
    //StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row);
}

// void StorageAuditTableManager::createAuditRowWidgets(const StorageAuditRow& row, int rowIx) {
//     const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
//     if (!mat)
//         return;

//     // 🧱 Anyag cella – szín + név
//     TableUtils::setMaterialNameCell(_table, rowIx, AuditTableColumns::Material,
//                                     mat->name,
//                                     mat->color.color(),
//                                     mat->color.name());

//     // 🧱 Üres cellák létrehozása – ezekbe kerül majd a tartalom
//     _table->setItem(rowIx, AuditTableColumns::Storage, new QTableWidgetItem());
//     _table->setItem(rowIx, AuditTableColumns::Expected, new QTableWidgetItem());
//     _table->setItem(rowIx, AuditTableColumns::Missing, new QTableWidgetItem());
//     _table->setItem(rowIx, AuditTableColumns::Status, new QTableWidgetItem());
//     _table->setItem(rowIx, AuditTableColumns::Barcode, new QTableWidgetItem());

//     // 🔘 Interaktív cella – Leftover esetén rádiógombok
//     if (row.sourceType == AuditSourceType::Leftover) {
//         auto container = new QWidget();
//         auto layout = new QHBoxLayout(container);
//         layout->setContentsMargins(0, 0, 0, 0);

//         auto radioPresent = new QRadioButton("Van");
//         auto radioMissing = new QRadioButton("Nincs");

//         layout->addWidget(radioPresent);
//         layout->addWidget(radioMissing);
//         container->setLayout(layout);

//         radioPresent->setProperty(RowId_Key, row.rowId);
//         radioMissing->setProperty(RowId_Key, row.rowId);

//         connect(radioPresent, &QRadioButton::toggled, this, [radioPresent, this]() {
//             if (radioPresent->isChecked()) {
//                 QUuid rowId = radioPresent->property(RowId_Key).toUuid();
//                 emit leftoverPresenceChanged(rowId, AuditPresence::Present);
//             }
//         });

//         connect(radioMissing, &QRadioButton::toggled, this, [radioMissing, this]() {
//             if (radioMissing->isChecked()) {
//                 QUuid rowId = radioMissing->property(RowId_Key).toUuid();
//                 emit leftoverPresenceChanged(rowId, AuditPresence::Missing);
//             }
//         });

//         _table->setCellWidget(rowIx, AuditTableColumns::Actual, container);
//     } else {
//         // 🔢 SpinBox – interaktív mennyiség cella
//         QSpinBox* actualSpin = new QSpinBox();
//         actualSpin->setRange(0, 9999);
//         actualSpin->setProperty(RowId_Key, row.rowId);
//         _table->setCellWidget(rowIx, AuditTableColumns::Actual, actualSpin);

//         connect(actualSpin, &QSpinBox::valueChanged, this, [actualSpin, this]() {
//             QUuid rowId = actualSpin->property(RowId_Key).toUuid();
//             emit auditValueChanged(rowId, actualSpin->value());
//         });
//     }
// }

// void StorageAuditTableManager::populateAuditRowContent(const StorageAuditRow& row, int rowIx, const QString& groupLabel) {
//     const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
//     if (!mat)
//         return;

//     // 🧩 Szöveges cellák feltöltése
//     if (auto* item = _table->item(rowIx, AuditTableColumns::Storage))
//         item->setText(row.storageName);

//     if (auto* item = _table->item(rowIx, AuditTableColumns::Expected))
//         item->setText(AuditCellFormatter::formatExpectedQuantity(row, groupLabel));

//     if (auto* item = _table->item(rowIx, AuditTableColumns::Missing))
//         item->setText(AuditCellFormatter::formatMissingQuantity(row));

//     if (auto* item = _table->item(rowIx, AuditTableColumns::Status))
//         item->setText(TableUtils::AuditCells::statusText(row));

//     if (auto* item = _table->item(rowIx, AuditTableColumns::Barcode))
//         item->setText(row.barcode);

//     // 🔘 Interaktív cella frissítése – Leftover esetén rádiógombok
//     if (row.sourceType == AuditSourceType::Leftover) {
//         auto container = qobject_cast<QWidget*>(_table->cellWidget(rowIx, AuditTableColumns::Actual));
//         if (container) {
//             auto radios = container->findChildren<QRadioButton*>();
//             for (auto* radio : radios) {
//                 if (radio->text() == "Van")
//                     radio->setChecked(row.actualQuantity > 0);
//                 else if (radio->text() == "Nincs")
//                     radio->setChecked(row.actualQuantity == 0);
//             }
//         }
//     } else {
//         // 🔢 SpinBox érték frissítése
//         auto* actualSpin = qobject_cast<QSpinBox*>(_table->cellWidget(rowIx, AuditTableColumns::Actual));
//         if (actualSpin)
//             actualSpin->setValue(row.actualQuantity);
//     }

//     // 🎨 Stílus és tooltip újraalkalmazása
//     //StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);
//     //StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row);
// }

void StorageAuditTableManager::clearTable() {
    _table->clearContents();
    _table->setRowCount(0);
    _rows.clear();
    _groupLabeler.clear();
}

// #include "storageaudittable_manager.h"
// #include "common/logger.h"
// #include "common/tableutils/storageaudittable_rowstyler.h"
// #include "common/tableutils/tableutils.h"
// //#include "model/storageaudit/auditcontext_text.h"
// #include "common/tableutils/tableutils_auditcells.h"
// #include "model/storageaudit/storageauditentry.h"
// #include "model/storageaudit/storageauditrow.h"

// #include <QCheckBox>
// #include <QRadioButton>
// #include <QSpinBox>

// #include <model/registries/materialregistry.h>
// #include "common/tableutils/auditcellformatter.h"

// StorageAuditTableManager::StorageAuditTableManager(QTableWidget* table, QWidget* parent)
//     : QObject(parent), _table(table), _parent(parent){
//     _groupSync = std::make_unique<TableUtils::AuditGroupSynchronizer>(
//         _table,
//         _auditRowMap,
//         _rows.rowIndexMap(),
//         &_groupLabeler,
//         this // 🔹 ez a hiányzó manager paraméter
//         );
// }

// void StorageAuditTableManager::addRow(const StorageAuditRow& row) {
//     if (!_table)
//         return;

//     const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
//     if (!mat)
//         return;

//     int rowIx = _table->rowCount();
//     _table->insertRow(rowIx);
//     _auditRowMap[row.rowId] = row;

//     createAuditRowWidgets(row, rowIx);
//     QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";
//     populateAuditRowContent(row, rowIx, groupLabel);

//     _rows.registerRow(rowIx, row.rowId); // opcionálisan extra: entry.storageId

//     if (row.context)
//         _groupSync->syncGroup(*row.context, row.rowId);

//     StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);     // 🎨 Stílus
//     StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row); // 🧠 Tooltip

//     if (row.context) {
//         zInfo(L("AuditContext [%1]: expected=%2, actual=%3, rows=%4")
//                    .arg(row.materialId.toString())
//                    .arg(row.context->totalExpected)
//                    .arg(row.context->totalActual)
//                    .arg(row.context->group.size()));
//     }

// }

// void StorageAuditTableManager::updateRow(const StorageAuditRow& row) {
//     if (!_table)
//         return;

//     const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
//     if (!mat)
//         return;

//     // 🧠 AuditContext naplózása – csak ha van
//     if (row.context) {
//         zInfo(L("🔄 Frissített AuditContext [%1]: expected=%2, actual=%3, rows=%4")
//                    .arg(row.materialId.toString())
//                    .arg(row.context->totalExpected)
//                    .arg(row.context->totalActual)
//                    .arg(row.context->group.size()));
//     }

//     // 🔍 Sorindex lekérése
//     std::optional<int> rowIxOpt = _rows.rowOf(row.rowId);
//     if (!rowIxOpt.has_value())
//         return;

//     int rowIx = rowIxOpt.value();

//     // 🗂️ Soradat frissítése a map-ben
//     _auditRowMap[row.rowId] = row;

//     // 🏷️ Csoportcímke lekérése
//     QString groupLabel = row.context ? _groupLabeler.labelFor(row.context.get()) : "";

//     // 🧩 Cellák tartalommal való feltöltése
//     populateAuditRowContent(row, rowIx, groupLabel);

//     // 🔁 Csoport szinkronizálása (kivéve az aktuális sort)
//     if (row.context)
//         _groupSync->syncGroup(*row.context, row.rowId);

//     // 🎨 Stílus és tooltip újraalkalmazása
//     StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);     // 🎨 Stílus
//     StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row); // 🧠 Tooltip

// }

// /**/

// void StorageAuditTableManager::createAuditRowWidgets(const StorageAuditRow& row, int rowIx) {
//     const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
//     if (!mat)
//         return;

//     TableUtils::setMaterialNameCell(_table, rowIx, ColMaterial,
//                                     mat->name,
//                                     mat->color.color(),
//                                     mat->color.name());

//     _table->setItem(rowIx, ColStorage, new QTableWidgetItem());
//     _table->setItem(rowIx, ColExpected, new QTableWidgetItem());
//     _table->setItem(rowIx, ColMissing, new QTableWidgetItem());
//     _table->setItem(rowIx, ColStatus, new QTableWidgetItem());
//     _table->setItem(rowIx, ColBarcode, new QTableWidgetItem());

//     if (row.sourceType == AuditSourceType::Leftover) {
//         auto container = new QWidget();
//         auto layout = new QHBoxLayout(container);
//         layout->setContentsMargins(0, 0, 0, 0);

//         auto radioPresent = new QRadioButton("Van");
//         auto radioMissing = new QRadioButton("Nincs");

//         layout->addWidget(radioPresent);
//         layout->addWidget(radioMissing);
//         container->setLayout(layout);

//         radioPresent->setProperty(RowId_Key, row.rowId);
//         radioMissing->setProperty(RowId_Key, row.rowId);

//         connect(radioPresent, &QRadioButton::toggled, this, [radioPresent, this]() {
//             if (radioPresent->isChecked()) {
//                 QUuid rowId = radioPresent->property(RowId_Key).toUuid();
//                 emit leftoverPresenceChanged(rowId, AuditPresence::Present);
//             }
//         });

//         connect(radioMissing, &QRadioButton::toggled, this, [radioMissing, this]() {
//             if (radioMissing->isChecked()) {
//                 QUuid rowId = radioMissing->property(RowId_Key).toUuid();
//                 emit leftoverPresenceChanged(rowId, AuditPresence::Missing);
//             }
//         });

//         _table->setCellWidget(rowIx, ColActual, container);
//     } else {
//         QSpinBox* actualSpin = new QSpinBox();
//         actualSpin->setRange(0, 9999);
//         actualSpin->setProperty(RowId_Key, row.rowId);
//         _table->setCellWidget(rowIx, ColActual, actualSpin);

//         connect(actualSpin, &QSpinBox::valueChanged, this, [actualSpin, this]() {
//             QUuid rowId = actualSpin->property(RowId_Key).toUuid();
//             emit auditValueChanged(rowId, actualSpin->value());
//         });
//     }
// }

// void StorageAuditTableManager::populateAuditRowContent(const StorageAuditRow& row, int rowIx, const QString& groupLabel) {
//     const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
//     if (!mat)
//         return;

//     if (auto* item = _table->item(rowIx, ColStorage))
//         item->setText(row.storageName);

//     if (auto* item = _table->item(rowIx, ColExpected))
//         item->setText(AuditCellFormatter::formatExpectedQuantity(row, groupLabel));

//     if (auto* item = _table->item(rowIx, ColMissing))
//         item->setText(AuditCellFormatter::formatMissingQuantity(row));

//     if (auto* item = _table->item(rowIx, ColStatus))
//         item->setText(TableUtils::AuditCells::statusText(row));

//     if (auto* item = _table->item(rowIx, ColBarcode))
//         item->setText(row.barcode);

//     if (row.sourceType == AuditSourceType::Leftover) {
//         auto container = qobject_cast<QWidget*>(_table->cellWidget(rowIx, ColActual));
//         if (container) {
//             auto radios = container->findChildren<QRadioButton*>();
//             for (auto* radio : radios) {
//                 if (radio->text() == "Van")
//                     radio->setChecked(row.actualQuantity > 0);
//                 else if (radio->text() == "Nincs")
//                     radio->setChecked(row.actualQuantity == 0);
//             }
//         }
//     } else {
//         auto* actualSpin = qobject_cast<QSpinBox*>(_table->cellWidget(rowIx, ColActual));
//         if (actualSpin)
//             actualSpin->setValue(row.actualQuantity);
//     }

//     StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);
//     StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row);
// }

// void StorageAuditTableManager::clearTable() {
//     _table->clearContents();
//     _table->setRowCount(0);
//     _rows.clear();
//     _groupLabeler.clear();
// }
