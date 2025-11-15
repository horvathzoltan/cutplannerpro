#include "stocktable_manager.h"
#include "../tableutils/stocktable_rowstyler.h"
#include "../tableutils/tableutils.h"
#include "materials/utils/material_utils.h"
#include "materials/utils/material_group_utils.h"
//#include "common/tableutils/resulttable_rowstyler.h"
#include "materials/registry/material_registry.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include "../../model/registries/stockregistry.h"
#include "../../model/registries/storageregistry.h"

StockTableManager::StockTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), _parent(parent)
    //,_rowId(table, ColName)
{}

void StockTableManager::addRow(const StockEntry& entry) {
    if (!_table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
    if (!mat)
        return;

    int rowIx = _table->rowCount();
    _table->insertRow(rowIx);

    //📛 Név + id    
    TableUtils::setMaterialNameCell(_table, rowIx, ColName,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name());

    //_rowId.set(rowIx, entry.entryId);
    _rows.registerRow(rowIx, entry.entryId); // opcionálisan extra: entry.storageId

    // 🧾 BarCode
    auto* itemBarcode = new QTableWidgetItem(mat->barcode);
    itemBarcode->setTextAlignment(Qt::AlignCenter);
    _table->setItem(rowIx, ColBarcode, itemBarcode);

    // 📐 Shape
    auto* itemShape = new QTableWidgetItem(MaterialUtils::formatShapeText(*mat));
    itemShape->setTextAlignment(Qt::AlignCenter);
    _table->setItem(rowIx, ColShape, itemShape);

    // 📏 Length
    auto* itemLength = new QTableWidgetItem(QString::number(mat->stockLength_mm));
    itemLength->setTextAlignment(Qt::AlignCenter);
    //itemLength->setData(Qt::UserRole, mat->stockLength_mm);
    _table->setItem(rowIx, ColLength, itemLength);

    // 🏷️ Mennyiség panel
    auto* quantityPanel = TableUtils::createQuantityCell(entry.quantity, entry.entryId, this, [this, entry]() {
        emit editQtyRequested(entry.entryId);
    });
    _table->setCellWidget(rowIx, ColQuantity, quantityPanel);
    //quantityPanel->setToolTip(QString("Mennyiség: %1").arg(entry.quantity));


    // 🏷️ Storage name
    const auto* storage = StorageRegistry::instance().findById(entry.storageId);
    QString storageName = storage ? storage->name : "—";   

    auto* storagePanel = TableUtils::createStorageCell(storageName, entry.entryId, this, [this, entry]() {
        emit editStorageRequested(entry.entryId);
    });
    _table->setCellWidget(rowIx, ColStorageName, storagePanel);
    //storagePanel->setToolTip(QString("Tároló: %1").arg(storageName));

    // 🏷️ Komment panel
    auto* commentPanel = TableUtils::createCommentCell(entry.comment, entry.entryId, this, [this, entry]() {
        emit editCommentRequested(entry.entryId);
    });
    _table->setCellWidget(rowIx, ColComment, commentPanel);


    // 🎯 CuttingMode
    // auto* itemCuttingMode = new QTableWidgetItem(CuttingModeUtils::toString(mat->cuttingMode));
    // itemCuttingMode->setTextAlignment(Qt::AlignCenter);
    // _table->setItem(rowIx, ColCuttingMode, itemCuttingMode);

    // // 🎨 PaintingMode
    // auto* itemPaintingMode = new QTableWidgetItem(PaintingModeUtils::toString(mat->paintingMode));
    // itemPaintingMode->setTextAlignment(Qt::AlignCenter);
    // _table->setItem(rowIx, ColPaintingMode, itemPaintingMode);

    // 🗑️ Törlés gomb
    QPushButton* btnDelete = TableUtils::createIconButton("🗑️", "Sor törlése", entry.entryId);    
    QPushButton* btnMove = TableUtils::createIconButton("➡️", "Mozgatás", entry.entryId);

    // 🧩 Panelbe csomagolás
    auto* actionPanel = new QWidget();
    auto* layout = new QHBoxLayout(actionPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    //layout->addWidget(btnUpdate);
    layout->addWidget(btnDelete);    
    layout->addWidget(btnMove);

    _table->setCellWidget(rowIx, ColAction, actionPanel);
    _table->setColumnWidth(ColAction, 64);

    QObject::connect(btnDelete, &QPushButton::clicked, this, [btnDelete, this]() {
        QUuid entryId = btnDelete->property(EntryId_Key).toUuid();
        emit deleteRequested(entryId);
    });

    QObject::connect(btnMove, &QPushButton::clicked, this, [btnMove, this]() {
        QUuid entryId = btnMove->property(EntryId_Key).toUuid();
        emit moveRequested(entryId);  // vagy akár külön signal: moveRequested(entryId);
    });

    StockTable::RowStyler::applyStyle(_table, rowIx, mat->stockLength_mm, entry.quantity, mat);
}

void StockTableManager::updateRow(const StockEntry& entry) {
    if (!_table)
        return;

    std::optional<int> rowIxOpt = _rows.rowOf(entry.entryId);

    if (!rowIxOpt.has_value())
        return;
    int rowIx = rowIxOpt.value();
  //  for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {
        //QUuid currentId = _rowId.get(rowIx);

       // if (currentId == entry.entryId) {
            const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
            if(!mat)
                return;

            QString materialName = mat ? mat->name : "(ismeretlen)";
            QString barcode = mat ? mat->barcode : "—";
            QString shape = mat ? MaterialUtils::formatShapeText(*mat) : "—";

            // 📛 Név
            TableUtils::setMaterialNameCell(_table, rowIx, ColName,
                                            mat->name,
                                            mat->color.color(),
                                            mat->color.name());
            // ez kiüti az előző cellwidgetet, de a RowId is megy vele

            // 🧾 Barcode
            auto* itemBarcode = _table->item(rowIx, ColBarcode);
            if (itemBarcode) itemBarcode->setText(barcode);

            // 📐 Shape
            auto* itemShape = _table->item(rowIx, ColShape);
            if (itemShape) itemShape->setText(shape);

            // 📏 Length
            auto* itemLength = _table->item(rowIx, ColLength);
            if (itemLength && mat) {
                itemLength->setText(QString::number(mat->stockLength_mm));
            }         

            // 🧾 Mennyiség panel
            auto* quantityPanel = _table->cellWidget(rowIx, ColQuantity);
            TableUtils::updateQuantityCell(quantityPanel, entry.quantity, entry.entryId);

            // 🏷️ Storage name        
            auto* storagePanel = _table->cellWidget(rowIx, ColStorageName);
            const auto* storage = StorageRegistry::instance().findById(entry.storageId);
            QString storageName = storage ? storage->name : "—";
            TableUtils::updateStorageCell(storagePanel, storageName, entry.entryId);


            auto* commentPanel = _table->cellWidget(rowIx, ColComment);
            TableUtils::updateCommentCell(commentPanel, entry.comment, entry.entryId);

            // 🎨 Stílus újraalkalmazás
            StockTable::RowStyler::applyStyle(_table, rowIx, mat->stockLength_mm, entry.quantity, mat);
          //  return;
        //}
  //  }

    //qWarning() << "⚠️ updateRow: Nem található sor a következő azonosítóval:" << entry.entryId;
}

void StockTableManager::refresh_TableFromRegistry()
{
    if (!_table)
        return;

    // 🧹 Tábla törlése
    TableUtils::clearSafely(_table);
    _rows.clear();

    const auto& stockEntries = StockRegistry::instance().readAll();
    const MaterialRegistry& materialReg = MaterialRegistry::instance();

    for (const auto& entry : stockEntries)
    {
        const MaterialMaster* master = materialReg.findById(entry.materialId);
        if (!master)
            continue;

        addRow(entry);  // addRow regisztrálja az új sort
    }

    //table->resizeColumnsToContents();
}

void StockTableManager::removeRowById(const QUuid& stockId) {    
    if (auto rowIxOpt = _rows.rowOf(stockId)) {
        int rowIx = *rowIxOpt;
        _table->removeRow(rowIx);
        _rows.unregisterRowByIndex(rowIx);
        _rows.syncAfterRemove(rowIx);
        return;
    }
}



