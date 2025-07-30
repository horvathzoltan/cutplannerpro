#include "stocktablemanager.h"
#include "common/materialutils.h"
#include "common/rowstyler.h"
//#include "common/grouputils.h"
#include "model/registries/materialregistry.h"
#include <QMessageBox>
#include "model/registries/stockregistry.h"

StockTableManager::StockTableManager(QTableWidget* table, QWidget* parent)
    : table(table), parent(parent) {}

void StockTableManager::addRow(const StockEntry& entry) {
    if (!table)
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
    if (!mat)
        return;

    int row = table->rowCount();
    table->insertRow(row);

    // 📛 Név + id
    auto* itemName = new QTableWidgetItem(mat->name);
    itemName->setTextAlignment(Qt::AlignCenter);
    itemName->setData(Qt::UserRole, mat->id);
    table->setItem(row, ColName, itemName);

    // 🧾 BarCode
    auto* itemBarcode = new QTableWidgetItem(mat->barcode);
    itemBarcode->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, ColBarcode, itemBarcode);

    // 📐 Shape
    auto* itemShape = new QTableWidgetItem(MaterialUtils::formatShapeText(*mat));
    itemShape->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, ColShape, itemShape);

    // 📏 Length
    auto* itemLength = new QTableWidgetItem(QString::number(mat->stockLength_mm));
    itemLength->setTextAlignment(Qt::AlignCenter);
    itemLength->setData(Qt::UserRole, mat->stockLength_mm);
    table->setItem(row, ColLength, itemLength);

    // 🔢 Quantity
    auto* itemQty = new QTableWidgetItem(QString::number(entry.quantity));
    itemQty->setTextAlignment(Qt::AlignCenter);
    itemQty->setData(Qt::UserRole, entry.quantity);
    table->setItem(row, ColQuantity, itemQty);

    RowStyler::applyStockStyle(table, row, mat);
}

void StockTableManager::updateRow(int row, const StockEntry& entry) {
    if (!table || row < 0 || row >= table->rowCount())
        return;

    const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
    if (!mat)
        return;

    // 📛 Név
    auto* itemName = table->item(row, ColName);
    if (itemName) itemName->setText(mat->name);

    // 🧾 Barcode
    auto* itemBarcode = table->item(row, ColBarcode);
    if (itemBarcode) itemBarcode->setText(mat->barcode);

    // 📐 Shape
    auto* itemShape = table->item(row, ColShape);
    if (itemShape) itemShape->setText(MaterialUtils::formatShapeText(*mat));

    // 📏 Length
    auto* itemLength = table->item(row, ColLength);
    if (itemLength) {
        itemLength->setText(QString::number(mat->stockLength_mm));
        itemLength->setData(Qt::UserRole, mat->stockLength_mm);
    }

    // 🔢 Quantity
    auto* itemQty = table->item(row, ColQuantity);
    if (itemQty) {
        itemQty->setText(QString::number(entry.quantity));
        itemQty->setData(Qt::UserRole, entry.quantity);
    }

    RowStyler::applyStockStyle(table, row, mat);
}


void StockTableManager::updateTableFromRegistry()
{
    if (!table)
        return;

    table->clearContents();
    table->setRowCount(0);

    const auto& stockEntries = StockRegistry::instance().all();
    const MaterialRegistry& materialReg = MaterialRegistry::instance();

    for (const auto& entry : stockEntries)
    {
        const MaterialMaster* master = materialReg.findById(entry.materialId);
        if (!master)
            continue;

        addRow(entry);  // 🔄 új metódus, ami az egész StockEntry-t feldolgozza
    }

    table->resizeColumnsToContents();
}


