#include "stocktable_manager.h"
#include "common/tableutils/stocktable_rowstyler.h"
#include "common/tableutils/tableutils.h"
#include "common/materialutils.h"
//#include "common/tableutils/resulttable_rowstyler.h"
#include "model/registries/materialregistry.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include "model/registries/stockregistry.h"
#include <model/registries/storageregistry.h>

StockTableManager::StockTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent),
    _rowId(table, ColName)
{}

void StockTableManager::addRow(const StockEntry& entry) {
    if (!table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
    if (!mat)
        return;

    int rowIx = table->rowCount();
    table->insertRow(rowIx);

    //📛 Név + id    
    TableUtils::setMaterialNameCell(table, rowIx, ColName,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name());

    _rowId.set(rowIx, entry.entryId);

    // 🧾 BarCode
    auto* itemBarcode = new QTableWidgetItem(mat->barcode);
    itemBarcode->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIx, ColBarcode, itemBarcode);

    // 📐 Shape
    auto* itemShape = new QTableWidgetItem(MaterialUtils::formatShapeText(*mat));
    itemShape->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIx, ColShape, itemShape);

    // 📏 Length
    auto* itemLength = new QTableWidgetItem(QString::number(mat->stockLength_mm));
    itemLength->setTextAlignment(Qt::AlignCenter);
    //itemLength->setData(Qt::UserRole, mat->stockLength_mm);
    table->setItem(rowIx, ColLength, itemLength);

    // 🏷️ Mennyiség panel
    auto* quantityPanel = TableUtils::createQuantityCell(entry.quantity, entry.entryId, this, [this, entry]() {
        emit editQtyRequested(entry.entryId);
    });
    table->setCellWidget(rowIx, ColQuantity, quantityPanel);
    //quantityPanel->setToolTip(QString("Mennyiség: %1").arg(entry.quantity));


    // 🏷️ Storage name
    const auto* storage = StorageRegistry::instance().findById(entry.storageId);
    QString storageName = storage ? storage->name : "—";   

    auto* storagePanel = TableUtils::createStorageCell(storageName, entry.entryId, this, [this, entry]() {
        emit editStorageRequested(entry.entryId);
    });
    table->setCellWidget(rowIx, ColStorageName, storagePanel);
    //storagePanel->setToolTip(QString("Tároló: %1").arg(storageName));

    // 🏷️ Komment panel
    auto* commentPanel = TableUtils::createCommentCell(entry.comment, entry.entryId, this, [this, entry]() {
        emit editCommentRequested(entry.entryId);
    });
    table->setCellWidget(rowIx, ColComment, commentPanel);

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

    table->setCellWidget(rowIx, ColAction, actionPanel);
    table->setColumnWidth(ColAction, 64);

    QObject::connect(btnDelete, &QPushButton::clicked, this, [btnDelete, this]() {
        QUuid entryId = btnDelete->property(EntryId_Key).toUuid();
        emit deleteRequested(entryId);
    });

    QObject::connect(btnMove, &QPushButton::clicked, this, [btnMove, this]() {
        QUuid entryId = btnMove->property(EntryId_Key).toUuid();
        emit moveRequested(entryId);  // vagy akár külön signal: moveRequested(entryId);
    });

    StockTable::RowStyler::applyStyle(table, rowIx, mat->stockLength_mm, entry.quantity, mat);
}

void StockTableManager::updateRow(const StockEntry& entry) {
    if (!table)
        return;

    for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {
        QUuid currentId = _rowId.get(rowIx);

        if (currentId == entry.entryId) {
            const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
            if(!mat) continue;

            QString materialName = mat ? mat->name : "(ismeretlen)";
            QString barcode = mat ? mat->barcode : "—";
            QString shape = mat ? MaterialUtils::formatShapeText(*mat) : "—";

            // 📛 Név
            TableUtils::setMaterialNameCell(table, rowIx, ColName,
                                            mat->name,
                                            mat->color.color(),
                                            mat->color.name());

            // 🧾 Barcode
            auto* itemBarcode = table->item(rowIx, ColBarcode);
            if (itemBarcode) itemBarcode->setText(barcode);

            // 📐 Shape
            auto* itemShape = table->item(rowIx, ColShape);
            if (itemShape) itemShape->setText(shape);

            // 📏 Length
            auto* itemLength = table->item(rowIx, ColLength);
            if (itemLength && mat) {
                itemLength->setText(QString::number(mat->stockLength_mm));
            }         

            // 🧾 Mennyiség panel
            auto* quantityPanel = table->cellWidget(rowIx, ColQuantity);
            TableUtils::updateQuantityCell(quantityPanel, entry.quantity, entry.entryId);

            // 🏷️ Storage name        
            auto* storagePanel = table->cellWidget(rowIx, ColStorageName);
            const auto* storage = StorageRegistry::instance().findById(entry.storageId);
            QString storageName = storage ? storage->name : "—";
            TableUtils::updateStorageCell(storagePanel, storageName, entry.entryId);


            auto* commentPanel = table->cellWidget(rowIx, ColComment);
            TableUtils::updateCommentCell(commentPanel, entry.comment, entry.entryId);

            // 🎨 Stílus újraalkalmazás
            StockTable::RowStyler::applyStyle(table, rowIx, mat->stockLength_mm, entry.quantity, mat);
            return;
        }
    }

    qWarning() << "⚠️ updateRow: Nem található sor a következő azonosítóval:" << entry.entryId;
}

void StockTableManager::refresh_TableFromRegistry()
{
    if (!table)
        return;

    // 🧹 Tábla törlése
    TableUtils::clearSafely(table);

    const auto& stockEntries = StockRegistry::instance().readAll();
    const MaterialRegistry& materialReg = MaterialRegistry::instance();

    for (const auto& entry : stockEntries)
    {
        const MaterialMaster* master = materialReg.findById(entry.materialId);
        if (!master)
            continue;

        addRow(entry);  // 🔄 új metódus, ami az egész StockEntry-t feldolgozza
    }

    //table->resizeColumnsToContents();
}

void StockTableManager::removeRowById(const QUuid& stockId) {
    for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {
        QTableWidgetItem* item = table->item(rowIx, ColName);
        if (!item) continue;

        QUuid currentId = _rowId.get(rowIx);
        if (currentId == stockId) {
            table->removeRow(rowIx);     // fő sor
            return;
        }
    }
}



