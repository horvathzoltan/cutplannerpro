#include "storageaudittable_manager.h"
#include "common/logger.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
#include "common/tableutils/tableutils.h"
#include "model/storageaudit/storageauditentry.h"
#include "model/storageaudit/storageauditrow.h"

#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>

#include <model/registries/materialregistry.h>

StorageAuditTableManager::StorageAuditTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), _parent(parent)
    //,_rowId(table, ColMaterial )
{}


void StorageAuditTableManager::addRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    if (!mat)
        return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    //table->setItem(r, ColMaterial, new QTableWidgetItem(mat->name));
    TableUtils::setMaterialNameCell(_table, rowIx, ColMaterial,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name()); // itt nincs egyedi entryId, csak materialId

    //_rowId.set(rowIx, row.rowId); // entryId beállítása
    _rows.registerRow(rowIx, row.rowId); // opcionálisan extra: entry.storageId

    //QTableWidgetItem *itemName = table->item(rowIx, ColMaterial);
    //itemName->setData(Qt::UserRole, mat->id);
    //itemName->setData(StockEntryIdIdRole, row.materialId);

    _table->setItem(rowIx, ColStorage, new QTableWidgetItem(row.storageName));
    //_table->setItem(rowIx, ColExpected, new QTableWidgetItem(QString::number(row.pickingQuantity)));
    QString pickingQuantity = row.pickingQuantity > 0
                      ? QString::number(row.pickingQuantity)
                      : QString();
    QString missingQuantity = row.pickingQuantity > 0
                                  ? QString::number(row.missingQuantity())
                                  : QString();
    _table->setItem(rowIx, ColExpected,new QTableWidgetItem(pickingQuantity));

    //const MaterialMaster* mat = MaterialRegistry::instance().findById(row.materialId);
    //QString barcode = row.barcode;

    // if (row.sourceType == AuditSourceType::Leftover) {
    //     const auto entry = LeftoverStockRegistry::instance().findById(row.stockEntryId);
    //     zInfo("Leftover");
    //     if (entry)
    //         barcode = entry->reusableBarcode(); // vagy entry->barcode
    // } else {
    //     zInfo("Stock");
    //         barcode = mat->barcode;
    // }

    _table->setItem(rowIx, ColBarcode, new QTableWidgetItem(row.barcode));

    QSpinBox* actualSpin = new QSpinBox();
    actualSpin->setRange(0, 9999);
    actualSpin->setValue(row.actualQuantity);
    actualSpin->setEnabled(true);
    actualSpin->setProperty(RowId_Key, row.rowId);
    _table->setCellWidget(rowIx, ColActual, actualSpin);

    //_table->setItem(rowIx, ColMissing, new QTableWidgetItem(QString::number(row.missingQuantity())));

    _table->setItem(rowIx, ColMissing,new QTableWidgetItem(missingQuantity));

    QTableWidgetItem* statusItem = new QTableWidgetItem();
    setStatusCell(statusItem, row.statusText());
    _table->setItem(rowIx, ColStatus, statusItem);

    QObject::connect(actualSpin, &QSpinBox::valueChanged, this, [actualSpin, this]() {
        QUuid rowId = actualSpin->property(RowId_Key).toUuid();
        int actualQuantity = actualSpin->value();

        emit auditValueChanged(rowId, actualQuantity);
    });

    if (row.sourceType == AuditSourceType::Leftover) {
        // QCheckBox* presentBox = new QCheckBox("Ott van");
        // presentBox->setChecked(row.actualQuantity > 0);
        // presentBox->setProperty(RowId_Key, row.rowId);
        // _table->setCellWidget(rowIx, ColActual, presentBox);

        // connect(presentBox, &QCheckBox::stateChanged, this, [presentBox, this]() {
        //     QUuid rowId = presentBox->property(RowId_Key).toUuid();
        //     bool isPresent = presentBox->isChecked();
        //     emit leftoverPresenceChanged(rowId, isPresent);
        // });
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

    StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);
    StorageAuditTable::RowStyler::applyTooltips(_table, rowIx, mat, row);
}

void StorageAuditTableManager::updateRow(const StorageAuditRow& row) {
    if (!_table)
        return;

    std::optional<int> rowIxOpt = _rows.rowOf(row.rowId);
    if (!rowIxOpt.has_value())
        return;
    int rowIx = rowIxOpt.value();

//    for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {
//        QUuid currentId = _rowId.get(rowIx);
//        if (currentId != row.rowId)
//            continue;

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

        QString pickingQuantity = row.pickingQuantity > 0
                                      ? QString::number(row.pickingQuantity)
                                      : QString();
        QString missingQuantity = row.pickingQuantity > 0
                                      ? QString::number(row.missingQuantity())
                                      : QString();

        // 🎯 Elvárt mennyiség
        auto* itemExpected = _table->item(rowIx, ColExpected);
        if (itemExpected)
            itemExpected->setText(pickingQuantity);

        // 📦 Tényleges mennyiség (SpinBox)
        auto* actualSpin = qobject_cast<QSpinBox*>(_table->cellWidget(rowIx, ColActual));
        if (actualSpin)
            actualSpin->setValue(row.actualQuantity);

        // ❌ Hiányzó mennyiség
        auto* itemMissing = _table->item(rowIx, ColMissing);
        if (itemMissing)
            itemMissing->setText(missingQuantity);

        // ✅ Státusz
        auto* statusItem = _table->item(rowIx, ColStatus);
        if (statusItem) {
            setStatusCell(statusItem, row.statusText());
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
        StorageAuditTable::RowStyler::applyStyle(_table, rowIx, mat, row);
       // return;
    //}

    //qWarning() << "⚠️ updateRow: Nem található audit sor a következő rowId-val:" << row.rowId;
}

void StorageAuditTableManager::setStatusCell(QTableWidgetItem* item, const QString& status) {
    if (!item)
        return;

    item->setText(status);

    if (status == "OK")
        item->setBackground(Qt::green);
    else if (status == "Hiányzik")
        item->setBackground(Qt::red);
    else
        item->setBackground(Qt::lightGray);
}


void StorageAuditTableManager::clearTable() {
    _table->clearContents();
    _table->setRowCount(0);
    _rows.clear();
}
