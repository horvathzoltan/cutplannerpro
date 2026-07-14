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
#include "model/storage/storageutils.h"
#include "view/tableutils/leftoverstyleutils.h"
#include <model/registries/leftoverstockregistry.h>

StockTableManager::StockTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), _table(table), _parent(parent)
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
    //const auto* storage = StorageRegistry::instance().findById(entry.storageId);
    //QString storageName = storage ? storage->name : "—";
    QString storageName = StorageRegistry::instance().uniqueHumanName(entry.storageId);

    auto* storagePanel = TableUtils::createStorageCell(storageName, entry.entryId, this, [this, entry]() {
        emit editStorageRequested(entry.entryId);
    });
    _table->setCellWidget(rowIx, ColStorageName, storagePanel);
    //storagePanel->setToolTip(QString("Tároló: %1").arg(storageName));

    QString storagePathTree = StorageUtils::buildPathTree(entry.storageId);
    storagePanel->setToolTip(storagePathTree);


    auto* itemCreated = new QTableWidgetItem(entry.createdAt.toString("yyyy-MM-dd HH:mm"));
    itemCreated->setTextAlignment(Qt::AlignCenter);
    _table->setItem(rowIx, ColCreatedAt, itemCreated);

    auto* itemSeen = new QTableWidgetItem(entry.lastSeenAt.toString("yyyy-MM-dd HH:mm"));
    itemSeen->setTextAlignment(Qt::AlignCenter);
    _table->setItem(rowIx, ColLastSeenAt, itemSeen);



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

    StockTable::RowStyler::applyStyle(_table, rowIx, mat->stockLength_mm, entry.quantity, mat, entry.lastSeenAt);
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
            //const auto* storage = StorageRegistry::instance().findById(entry.storageId);
            //QString storageName = storage ? storage->name : "—";
            QString storageName = StorageRegistry::instance().uniqueHumanName(entry.storageId);

            TableUtils::updateStorageCell(storagePanel, storageName, entry.entryId);
            QString storagePathTree = StorageUtils::buildPathTree(entry.storageId);
            storagePanel->setToolTip(storagePathTree);


            auto* itemCreated = _table->item(rowIx, ColCreatedAt);
            if (itemCreated)
                itemCreated->setText(entry.createdAt.toString("yyyy-MM-dd HH:mm"));

            auto* itemSeen = _table->item(rowIx, ColLastSeenAt);
            if (itemSeen)
                itemSeen->setText(entry.lastSeenAt.toString("yyyy-MM-dd HH:mm"));



            auto* commentPanel = _table->cellWidget(rowIx, ColComment);
            TableUtils::updateCommentCell(commentPanel, entry.comment, entry.entryId);

            // 🎨 Stílus újraalkalmazás
            StockTable::RowStyler::applyStyle(_table, rowIx, mat->stockLength_mm, entry.quantity, mat, entry.lastSeenAt);
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

void StockTableManager::highlight(const QUuid& id)
{
    auto rowOpt = _rows.rowOf(id);
    if (!rowOpt)
        return;

    int row = *rowOpt;

    // 2️⃣ Qt gyári kijelölés
    _table->selectRow(row);
    _table->clearSelection();

    // 3️⃣ Qt gyári fókusz
    if (_table->selectionMode() == QAbstractItemView::SingleSelection) {
        _table->selectRow(row);
    } else {
        _table->selectionModel()->select(
            _table->model()->index(row, 0),
            QItemSelectionModel::Select | QItemSelectionModel::Rows
            );
    }
    // 4️⃣ Görgetés
    _table->scrollToItem(_table->item(row, 0), QAbstractItemView::PositionAtCenter);

    _highlightedRow = row;
}

void StockTableManager::refresh_TableFiltered(const QSet<QUuid>& storageIds)
{
    TableUtils::clearSafely(_table);
    _rows.clear();

    const auto& stockEntries = StockRegistry::instance().readAll();

    for (const auto& entry : stockEntries)
    {
        if (!storageIds.contains(entry.storageId))
            continue;

        addRow(entry);   // RowTracker újraépül
    }

    const auto& leftovers = LeftoverStockRegistry::instance().readAll();
    for (const auto& e : leftovers)
    {
        if (storageIds.contains(e.storageId))
            addLeftoverRow(e);
    }

}

// void StockTableManager::addLeftoverRow(const LeftoverStockEntry& e)
// {
//     int row = _table->rowCount();
//     _table->insertRow(row);

//     // teljes sor colspan
//     _table->setSpan(row, 0, 1, _table->columnCount());

//     // fő cella – leftover információ
//     auto* item = new QTableWidgetItem(
//         QString("LEFTOVER: %1 | %2 mm")
//             .arg(e.barcode)
//             .arg(e.availableLength_mm)
//         );
//     item->setData(Qt::UserRole, e.entryId);          // leftover entryId
//     item->setData(Qt::UserRole + 1, "leftoverRow");  // típusjelölés
//     item->setForeground(Qt::gray);
//     item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

//     _table->setItem(row, 0, item);

//     // navigációs gomb
//     auto* btnNav = TableUtils::createIconButton("➡️", "Ugrás a leftoverre", e.entryId);

//     // panel a gombnak
//     auto* panel = new QWidget();
//     auto* layout = new QHBoxLayout(panel);
//     layout->setContentsMargins(0, 0, 0, 0);
//     layout->addWidget(btnNav);

//     _table->setCellWidget(row, 0, panel);

//     // jelzés a MainWindow felé
//     connect(btnNav, &QPushButton::clicked, this, [this, e]() {
//         emit leftoverNavigateRequested(e.entryId);
//     });
// }

void StockTableManager::addLeftoverRow(const LeftoverStockEntry& e)
{
    int row = _table->rowCount();
    _table->insertRow(row);

    // 1️⃣ Barcode cella
    auto* itemBarcode = new QTableWidgetItem(e.barcode);
    itemBarcode->setForeground(Qt::gray);
    itemBarcode->setTextAlignment(Qt::AlignCenter);
    itemBarcode->setData(Qt::UserRole, e.entryId);
    itemBarcode->setData(Qt::UserRole + 1, "leftoverRow");
    _table->setItem(row, ColBarcode, itemBarcode);

    // 2️⃣ Anyagnév + bogyós színwidget
    const MaterialMaster* mat = MaterialRegistry::instance().findById(e.materialId);
    QString materialName = mat ? mat->name : "(ismeretlen)";
    QColor materialColor = mat ? mat->color.color() : QColor(Qt::gray);
    QString colorTooltip = mat ? mat->color.name() : "";

    TableUtils::setMaterialNameCell(_table, row, ColName,
                                    materialName,
                                    materialColor,
                                    colorTooltip);

    // leftover sor szürke betűszín
    if (auto* w = _table->cellWidget(row, ColName)) {
        w->setStyleSheet("color: gray;");
    }

    // 3️⃣ Hossz cella
    auto* itemLength = new QTableWidgetItem(QString::number(e.availableLength_mm));
    itemLength->setForeground(Qt::gray);
    itemLength->setTextAlignment(Qt::AlignCenter);
    _table->setItem(row, ColLength, itemLength);

    // 4️⃣ CreatedAt cella
    auto* itemCreated = new QTableWidgetItem(e.createdAt.toString("yyyy-MM-dd HH:mm"));
    itemCreated->setForeground(Qt::gray);
    itemCreated->setTextAlignment(Qt::AlignCenter);
    _table->setItem(row, ColCreatedAt, itemCreated);

    // 5️⃣ LastSeenAt cella
    auto* itemSeen = new QTableWidgetItem(e.lastSeenAt.toString("yyyy-MM-dd HH:mm"));
    itemSeen->setForeground(Qt::gray);
    itemSeen->setTextAlignment(Qt::AlignCenter);
    _table->setItem(row, ColLastSeenAt, itemSeen);

    // 6️⃣ Navi gomb cella
    auto* btnNav = TableUtils::createIconButton("➡️", "Ugrás a leftoverre", e.entryId);

    auto* panel = new QWidget();
    auto* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(btnNav);

    _table->setCellWidget(row, ColAction, panel);

    connect(btnNav, &QPushButton::clicked, this, [this, e]() {
        emit leftoverNavigateRequested(e.entryId);
    });


    // -----------------------------
    // Prefix ellenőrzés (RSM / RST)
    // -----------------------------
    LeftoverStyleUtils::applyPrefixStyle(_table, row,
                                         StockTableManager::ColBarcode,
                                         e.barcode);

    LeftoverStyleUtils::applyAgeStyle(_table, row,
                                      StockTableManager::ColLastSeenAt,
                                      e.lastSeenAt);

}
