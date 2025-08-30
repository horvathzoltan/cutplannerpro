#include "storageaudittable_manager.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
#include "common/tableutils/tableutils.h"
#include "model/storageaudit/storageauditentry.h"
#include "model/storageaudit/storageauditrow.h"

#include <QCheckBox>
#include <QSpinBox>

#include <model/registries/materialregistry.h>

StorageAuditTableManager::StorageAuditTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent) {}


void StorageAuditTableManager::addRow(const StorageAuditRow& row) {
    if (!table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    int r = table->rowCount();
    table->insertRow(r);

    //table->setItem(r, ColMaterial, new QTableWidgetItem(mat->name));
    TableUtils::setMaterialNameCell(table, r, ColMaterial,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name(), // vagy mat->ralCode
                                    mat->id,
                                    row.materialId); // itt nincs egyedi entryId, csak materialId

    table->setItem(r, ColStorage, new QTableWidgetItem(row.storageName));
    table->setItem(r, ColExpected, new QTableWidgetItem(QString::number(row.pickingQuantity)));

    QSpinBox* actualSpin = new QSpinBox();
    actualSpin->setRange(0, 9999);
    actualSpin->setValue(row.actualQuantity);
    actualSpin->setEnabled(false);
    table->setCellWidget(r, ColActual, actualSpin);

    table->setItem(r, ColMissing, new QTableWidgetItem(QString::number(row.missingQuantity())));

    QTableWidgetItem* statusItem = new QTableWidgetItem(row.status());
    if (row.status() == "OK") statusItem->setBackground(Qt::green);
    else if (row.status() == "Hiányzik") statusItem->setBackground(Qt::red);
    else statusItem->setBackground(Qt::lightGray);

    table->setItem(r, ColStatus, statusItem);



    StorageAuditTable::RowStyler::applyStyle(table, r, mat, row);
}


// void StorageAuditTableManager::addRow_old(const StorageAuditEntry& entry) {
//     int row = table->rowCount();
//     table->insertRow(row);

//     table->setItem(row, ColMaterial, new QTableWidgetItem(entry.materialName));
//     table->setItem(row, ColStorage, new QTableWidgetItem(entry.storageName));
//     table->setItem(row, ColExpected, new QTableWidgetItem(QString::number(entry.expectedQuantity)));

//     // QCheckBox *presentBox = new QCheckBox();
//     // presentBox->setChecked(entry.isPresent);
//     // presentBox->setEnabled(false); // csak megjelenítés
//     // table->setCellWidget(row, ColPresent, presentBox);

//     QSpinBox* actualSpin = new QSpinBox();
//     actualSpin->setRange(0, 9999);
//     actualSpin->setValue(entry.actualQuantity);
//     actualSpin->setEnabled(false); // audit eredmény, nem szerkeszthető
//     table->setCellWidget(row, ColActual, actualSpin);

//     table->setItem(row, ColMissing, new QTableWidgetItem(QString::number(entry.missingQuantity)));

//     QTableWidgetItem* statusItem = new QTableWidgetItem(entry.status);
//     if (entry.status == "OK") statusItem->setBackground(Qt::green);
//     else if (entry.status == "Hiányzik") statusItem->setBackground(Qt::red);
//     else statusItem->setBackground(Qt::lightGray);


//     table->setItem(row, ColStatus, statusItem);
// }

void StorageAuditTableManager::clearTable() {
    table->clearContents();
    table->setRowCount(0);
}
