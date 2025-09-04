#include "storageaudittable_manager.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
#include "common/tableutils/tableutils.h"
#include "model/storageaudit/storageauditentry.h"
#include "model/storageaudit/storageauditrow.h"

#include <QCheckBox>
#include <QSpinBox>

#include <model/registries/materialregistry.h>

StorageAuditTableManager::StorageAuditTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent),
    _rowId(table, ColMaterial ){}


void StorageAuditTableManager::addRow(const StorageAuditRow& row) {
    if (!table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    int rowIx = table->rowCount();
    table->insertRow(rowIx);

    //table->setItem(r, ColMaterial, new QTableWidgetItem(mat->name));
    TableUtils::setMaterialNameCell(table, rowIx, ColMaterial,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name()); // itt nincs egyedi entryId, csak materialId

    _rowId.set(rowIx, mat->id);

    //QTableWidgetItem *itemName = table->item(rowIx, ColMaterial);
    //itemName->setData(Qt::UserRole, mat->id);
    //itemName->setData(StockEntryIdIdRole, row.materialId);

    table->setItem(rowIx, ColStorage, new QTableWidgetItem(row.storageName));
    table->setItem(rowIx, ColExpected, new QTableWidgetItem(QString::number(row.pickingQuantity)));

    QSpinBox* actualSpin = new QSpinBox();
    actualSpin->setRange(0, 9999);
    actualSpin->setValue(row.actualQuantity);
    actualSpin->setEnabled(true);
    actualSpin->setProperty(EntryId_Key, row.entryId);
    table->setCellWidget(rowIx, ColActual, actualSpin);

    table->setItem(rowIx, ColMissing, new QTableWidgetItem(QString::number(row.missingQuantity())));

    QTableWidgetItem* statusItem = new QTableWidgetItem(row.status());
    if (row.status() == "OK") statusItem->setBackground(Qt::green);
    else if (row.status() == "Hiányzik") statusItem->setBackground(Qt::red);
    else statusItem->setBackground(Qt::lightGray);

    table->setItem(rowIx, ColStatus, statusItem);

    QObject::connect(actualSpin, &QSpinBox::valueChanged, this, [actualSpin, this]() {
        QUuid entryId = actualSpin->property(EntryId_Key).toUuid();
        int actualQuantity = actualSpin->value();

        emit auditValueChanged(entryId, actualQuantity);
    });

    StorageAuditTable::RowStyler::applyStyle(table, rowIx, mat, row);
}

void StorageAuditTableManager::clearTable() {
    table->clearContents();
    table->setRowCount(0);
}
