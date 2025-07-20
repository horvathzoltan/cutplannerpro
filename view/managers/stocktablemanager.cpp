#include "stocktablemanager.h"
#include "common/materialutils.h"
#include "common/rowstyler.h"
//#include "common/grouputils.h"
#include "model/materialregistry.h"
#include <QMessageBox>
#include <model/stockregistry.h>

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

std::optional<StockEntry> StockTableManager::readRow(int row) const {
    if (!table || row < 0 || row >= table->rowCount())
        return std::nullopt;

    auto* itemName = table->item(row, ColName);
    auto* itemQty  = table->item(row, ColQuantity);

    if (!itemName || !itemQty)
        return std::nullopt;

    QUuid materialId;
    int quantity = 0;

    const QVariant idData  = itemName->data(Qt::UserRole);
    const QVariant qtyData = itemQty->data(Qt::UserRole);

    if (idData.canConvert<QUuid>())
        materialId = idData.toUuid();

    if (qtyData.canConvert<int>())
        quantity = qtyData.toInt();

    if (materialId.isNull() || quantity <= 0)
        return std::nullopt;

    return StockEntry { materialId, quantity };
}

void StockTableManager::updateTableFromRepository()
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


