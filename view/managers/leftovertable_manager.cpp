#include "leftovertable_manager.h"
#include "../tableutils/tableutils.h"
#include "materials/utils/material_utils.h"
//#include "model/material/materialgroup_utils.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include "../tableutils/leftovertable_rowstyler.h"
#include "../../model/registries/leftoverstockregistry.h"
#include "../../model/registries/storageregistry.h"
#include "materials/registry/material_registry.h"
#include <view/dialog/waste/scrapbybarcodedialog.h>

LeftoverTableManager::LeftoverTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent)
{}

void LeftoverTableManager::addRow(const LeftoverStockEntry& entry) {
    if (!table)
        return;

    const MaterialMaster* mat = entry.master();
    if (!mat)
        return;

    int rowIx = table->rowCount();
    table->insertRow(rowIx);

    // 📛 Név + id
    TableUtils::setMaterialNameCell(table, rowIx, ColName,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name());
    //_rowId.set(rowIx,entry.entryId);
    _rows.registerRow(rowIx, entry.entryId); // opcionálisan extra: entry.storageId

    // 🧾 Vonalkód
    auto* itemBarcode = new QTableWidgetItem(mat ? mat->barcode : "-");
    itemBarcode->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIx, ColBarcode, itemBarcode);

    auto* itemID = new QTableWidgetItem(entry.barcode);
    itemID->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIx, ColReusableId, itemID);

    // 📏 Hossz
    auto* itemLength = new QTableWidgetItem(QString::number(entry.availableLength_mm));
    itemLength->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIx, ColLength, itemLength);

    // 📐 Forma
    auto* itemShape = new QTableWidgetItem(mat ? MaterialUtils::formatShapeText(*mat) : "-");
    itemShape->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIx, ColShape, itemShape);

    // 🛠️ Forrás
    auto* itemSource = new QTableWidgetItem(entry.sourceAsString());
    itemSource->setTextAlignment(Qt::AlignCenter);
    table->setItem(rowIx, ColSource, itemSource);

    // ♻️ Újrahasználhatóság
    auto* itemReusable = new QTableWidgetItem();
    TableUtils::setReusableCell(itemReusable, entry.availableLength_mm);
    table->setItem(rowIx, ColReusable, itemReusable);

    // 🏷️ Storage name
    auto* storageOpt = StorageRegistry::instance().findById(entry.storageId);
    QString storageName = storageOpt ? storageOpt->name : "—";

    auto* storagePanel = TableUtils::createStorageCell(storageName, entry.entryId, this, [this, entry]() {
        emit editStorageRequested(entry.entryId);  // 🔔 új signal
    });
    table->setCellWidget(rowIx, ColStorageName, storagePanel);

    // 🗑️ Törlés gomb
    QPushButton* btnDelete = new QPushButton("🗑️");
    btnDelete->setToolTip("Törlés");
    btnDelete->setFixedSize(28, 28);
    btnDelete->setStyleSheet("QPushButton { border: none; }");
    btnDelete->setProperty(EntryId_Key, entry.entryId);

    // ✏️ Szerkesztés gomb
    QPushButton* btnEdit = new QPushButton("✏️");
    btnEdit->setToolTip("Szerkesztés");
    btnEdit->setFixedSize(28, 28);
    btnEdit->setStyleSheet("QPushButton { border: none; }");
    btnEdit->setProperty(EntryId_Key, entry.entryId);

    // Panelbe helyezés
    auto* actionPanel = new QWidget();
    auto* layout = new QHBoxLayout(actionPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(btnEdit);
    layout->addWidget(btnDelete);

    table->setCellWidget(rowIx, ColActions, actionPanel);
    table->setColumnWidth(ColActions, 64);

    // 📡 Signal kapcsolás UUID alapú
    QObject::connect(btnDelete, &QPushButton::clicked, this, [btnDelete, this]() {
        QUuid entryId = btnDelete->property(EntryId_Key).toUuid();
        emit deleteRequested(entryId);
    });

    QObject::connect(btnEdit, &QPushButton::clicked, this, [btnEdit, this]() {
        QUuid entryId = btnEdit->property(EntryId_Key).toUuid();
        emit editRequested(entryId);
    });

    // 🎨 Stílus
    LeftoverTable::RowStyler::applyStyle(table, rowIx, mat, entry);
}

void LeftoverTableManager::updateRow(const LeftoverStockEntry& entry) {
    if (!table)
        return;

    std::optional<int> rowIxOpt = _rows.rowOf(entry.entryId);

    if (!rowIxOpt.has_value())
        return;

    int rowIx = rowIxOpt.value();


    //zInfo("LeftoverTableManager::updateRow id:"+entry.entryId.toString());
    //for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {
      //  QUuid currentId = _rowId.get(rowIx);

        //zInfo("rowIx:"+QString::number(rowIx)+ "rowid:"+currentId.toString());
        //if (currentId == entry.entryId) {

      //      zInfo("updaterow ok");

            const MaterialMaster* mat = entry.master();
            if(!mat) return;

            QString materialName = mat ? mat->name : "(ismeretlen)";
            QString barcode = mat ? mat->barcode : "-";
            QString shape = mat ? MaterialUtils::formatShapeText(*mat) : "-";

            // 📛 Név
            TableUtils::setMaterialNameCell(table, rowIx, ColName,
                                            mat->name,
                                            mat->color.color(),
                                            mat->color.name());

            LeftoverTable::RowStyler::applyStyle(table, rowIx, mat, entry);

            // 🧾 Material vonalkód
            auto* itemBarcode = table->item(rowIx, ColBarcode);
            if (itemBarcode) itemBarcode->setText(barcode);

            // 🧾 Maradék vonalkód
            auto* itemID = table->item(rowIx, ColReusableId);
            if (itemID) itemID->setText(entry.barcode);

            // 📏 Hossz
            auto* itemLength = table->item(rowIx, ColLength);
            if (itemLength) {
                itemLength->setText(QString::number(entry.availableLength_mm));
            }

            // 📐 Alak
            auto* itemShape = table->item(rowIx, ColShape);
            if (itemShape) itemShape->setText(shape);

            // 🛠️ Forrás
            auto* itemSource = table->item(rowIx, ColSource);
            if (itemSource) {
                itemSource->setText(entry.sourceAsString());
            }

            // ♻️ Újrahasználhatóság
            auto* itemReusable = table->item(rowIx, ColReusable);
            TableUtils::setReusableCell(itemReusable, entry.availableLength_mm);

            // 🏷️ Storage name
            auto* storagePanel = table->cellWidget(rowIx, ColStorageName);
            if (storagePanel) {
                const auto* storage = StorageRegistry::instance().findById(entry.storageId);
                QString storageName = storage ? storage->name : "—";
                TableUtils::updateStorageCell(storagePanel, storageName, entry.entryId);
            }

          //  QUuid currentId2 = _rowId.get(rowIx);
          //  zInfo("rowIx:"+QString::number(rowIx)+ "currentId2:"+currentId2.toString());

            // 🎨 Sor stílus újraalkalmazása
            LeftoverTable::RowStyler::applyStyle(table, rowIx, mat, entry);

          //  QUuid currentId3 = _rowId.get(rowIx);
          //  zInfo("rowIx:"+QString::number(rowIx)+ "currentId3:"+currentId3.toString());
            return;
  //      }
//    }

  //  qWarning() << "⚠️ updateRow: Nem található sor azonosítóval:" << entry.entryId;
}



// void LeftoverTableManager::appendRows(const QVector<LeftoverStockEntry>& newResults) {
//     if (!table)
//         return;

//     for (const auto& e : newResults)
//         addRow(e);
// }



// void LeftoverTableManager::clear() {
//     table->clearContents();
//     table->setRowCount(0);
// }

void LeftoverTableManager::refresh_TableFromRegistry() {
    if (!table)
        return;

    // 🧹 Tábla törlése
    TableUtils::clearSafely(table);
    _rows.clear();

    const auto& stockEntries = LeftoverStockRegistry::instance().readAll();
    const MaterialRegistry& materialReg = MaterialRegistry::instance();

    for (const auto& entry : stockEntries) {
        const MaterialMaster* master = materialReg.findById(entry.materialId);
        if (!master)
            continue;

        addRow(entry);
    }

    //table->resizeColumnsToContents();
}


void LeftoverTableManager::removeRowById(const QUuid& id) {
    if (auto rowIxOpt = _rows.rowOf(id)) {
        int rowIx = *rowIxOpt;
        table->removeRow(rowIx);
        _rows.unregisterRowByIndex(rowIx);
        _rows.syncAfterRemove(rowIx);
        return;
    }
}

void LeftoverTableManager::highlight(const QUuid& id)
{
    auto rowOpt = _rows.rowOf(id);
    if (!rowOpt)
        return;

    int row = *rowOpt;

    table->setFocus();
    table->clearSelection();

    if (table->selectionMode() == QAbstractItemView::SingleSelection) {
        table->selectRow(row); // ez egy sort jelöl ki, a korábbi kijelöléseket törli
    } else {
        table->selectionModel()->select(
            table->model()->index(row, 0),
            QItemSelectionModel::Select | QItemSelectionModel::Rows
            );
    }

    table->scrollToItem(table->item(row, 0), QAbstractItemView::PositionAtCenter);

    _highlightedRow = row;
}

void LeftoverTableManager::openScrapDialog()
{
    ScrapByBarcodeDialog dlg;

    while (dlg.exec() == QDialog::Accepted)
    {
        QString code = dlg.barcode();
        if (code.isEmpty()) {
            if (!dlg.repeat())
                break;
            dlg.clearBarcodeField();
            continue;
        }

        // 1) Registry keresés
        auto entryOpt = LeftoverStockRegistry::instance().findByBarcode(code.trimmed());
        if (!entryOpt) {
            QMessageBox::warning(nullptr, "Not found",
                                 "No leftover found with this barcode.");
            if (!dlg.repeat())
                break;
            dlg.clearBarcodeField();
            continue;
        }

        QUuid id = entryOpt->entryId;

        // 2) highlight (opcionális)
        highlight(id);

        // 3) törlés → Presenter-en keresztül
        emit deleteRequested(id);

        // 4) repeat mód
        if (!dlg.repeat())
            break;

        dlg.clearBarcodeField();
    }
}
