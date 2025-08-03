#include "stocktablemanager.h"
#include "common/materialutils.h"
#include "common/rowstyler.h"
//#include "common/grouputils.h"
#include "model/registries/materialregistry.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include "model/registries/stockregistry.h"

StockTableManager::StockTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent) {}

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
    itemName->setData(StockEntryIdIdRole, entry.entryId);

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

    // 🗑️ Törlés gomb
    QPushButton* btnDelete = new QPushButton("🗑️");
    btnDelete->setToolTip("Sor törlése");
    btnDelete->setFixedSize(28, 28);
    btnDelete->setStyleSheet("QPushButton { border: none; }");
    btnDelete->setProperty("entryId", entry.entryId);

    // ✏️ Update gomb
    QPushButton* btnUpdate = new QPushButton("✏️");
    btnUpdate->setToolTip("Mennyiség módosítása");
    btnUpdate->setFixedSize(28, 28);
    btnUpdate->setStyleSheet("QPushButton { border: none; }");
    btnUpdate->setProperty("entryId", entry.entryId);

    // 🧩 Panelbe csomagolás
    auto* actionPanel = new QWidget();
    auto* layout = new QHBoxLayout(actionPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(btnUpdate);
    layout->addWidget(btnDelete);

    table->setCellWidget(row, ColAction, actionPanel);
    table->setColumnWidth(ColAction, 64);

    QObject::connect(btnDelete, &QPushButton::clicked, this, [btnDelete, this]() {
        QUuid entryId = btnDelete->property("entryId").toUuid();
        emit deleteRequested(entryId);
    });

    QObject::connect(btnUpdate, &QPushButton::clicked, this, [btnUpdate, this]() {
        QUuid id = btnUpdate->property("entryId").toUuid();
        emit editRequested(id);
    });

    RowStyler::applyStockStyle(table, row, mat, entry.quantity);
}

void StockTableManager::updateRow(const StockEntry& entry) {
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* itemName = table->item(row, ColName);
        if (!itemName)
            continue;

        QUuid currentId = itemName->data(StockEntryIdIdRole).toUuid(); // 👈 Saját role
        if (currentId == entry.entryId) {
            const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
            QString materialName = mat ? mat->name : "(ismeretlen)";
            QString barcode = mat ? mat->barcode : "—";
            QString shape = mat ? MaterialUtils::formatShapeText(*mat) : "—";

            // 📛 Név
            itemName->setText(materialName);
            itemName->setData(Qt::UserRole, QVariant::fromValue(entry.materialId));
            itemName->setData(StockEntryIdIdRole, entry.entryId);

            // 🧾 Barcode
            auto* itemBarcode = table->item(row, ColBarcode);
            if (itemBarcode) itemBarcode->setText(barcode);

            // 📐 Shape
            auto* itemShape = table->item(row, ColShape);
            if (itemShape) itemShape->setText(shape);

            // 📏 Length
            auto* itemLength = table->item(row, ColLength);
            if (itemLength && mat) {
                itemLength->setText(QString::number(mat->stockLength_mm));
                itemLength->setData(Qt::UserRole, mat->stockLength_mm);
            }

            // 🔢 Quantity
            auto* itemQty = table->item(row, ColQuantity);
            if (itemQty) {
                itemQty->setText(QString::number(entry.quantity));
                itemQty->setData(Qt::UserRole, entry.quantity);
            }

            // 🎨 Stílus újraalkalmazás
            RowStyler::applyStockStyle(table, row, mat, entry.quantity);
            return;
        }
    }

    qWarning() << "⚠️ updateRow: Nem található sor a következő azonosítóval:" << entry.entryId;
}


// void StockTableManager::updateRow(int row, const StockEntry& entry) {
//     if (!table || row < 0 || row >= table->rowCount())
//         return;

//     const MaterialMaster* mat = MaterialRegistry::instance().findById(entry.materialId);
//     if (!mat)
//         return;

//     // 📛 Név
//     auto* itemName = table->item(row, ColName);
//     if (itemName) itemName->setText(mat->name);

//     // 🧾 Barcode
//     auto* itemBarcode = table->item(row, ColBarcode);
//     if (itemBarcode) itemBarcode->setText(mat->barcode);

//     // 📐 Shape
//     auto* itemShape = table->item(row, ColShape);
//     if (itemShape) itemShape->setText(MaterialUtils::formatShapeText(*mat));

//     // 📏 Length
//     auto* itemLength = table->item(row, ColLength);
//     if (itemLength) {
//         itemLength->setText(QString::number(mat->stockLength_mm));
//         itemLength->setData(Qt::UserRole, mat->stockLength_mm);
//     }

//     // 🔢 Quantity
//     auto* itemQty = table->item(row, ColQuantity);
//     if (itemQty) {
//         itemQty->setText(QString::number(entry.quantity));
//         itemQty->setData(Qt::UserRole, entry.quantity);
//     }

//     RowStyler::applyStockStyle(table, row, mat);
// }


void StockTableManager::refresh_TableFromRegistry()
{
    if (!table)
        return;

    table->clearContents();
    table->setRowCount(0);

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
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* item = table->item(row, ColName);
        if (!item) continue;

        QUuid id = item->data(StockEntryIdIdRole).toUuid(); // egyedi role, ha van
        if (id == stockId) {
            table->removeRow(row);     // fő sor
            //table->removeRow(row);     // meta sor
            return;
        }
    }
}

