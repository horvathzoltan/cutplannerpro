#include "leftovertable_manager.h"
#include "common/tableutils/tableutils.h"
#include "common/materialutils.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <common/tableutils/leftovertable_rowstyler.h>
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/storageregistry.h>
#include "model/registries/materialregistry.h"

LeftoverTableManager::LeftoverTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent),
    _rowId(table, ColName ) {}

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
    _rowId.set(rowIx,entry.entryId);

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

    for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {
        QUuid currentId = _rowId.get(rowIx);

        if (currentId == entry.entryId) {
            const MaterialMaster* mat = entry.master();
            if(!mat) continue;

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

            // 🎨 Sor stílus újraalkalmazása
            LeftoverTable::RowStyler::applyStyle(table, rowIx, mat, entry);

            return;
        }
    }

    qWarning() << "⚠️ updateRow: Nem található sor azonosítóval:" << entry.entryId;
}



void LeftoverTableManager::appendRows(const QVector<LeftoverStockEntry>& newResults) {
    if (!table)
        return;

    for (const auto& e : newResults)
        addRow(e);
}

void LeftoverTableManager::removeRowById(const QUuid& id) {
    for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {

        QUuid currentId = _rowId.get(rowIx);
        if (currentId == id) {
            table->removeRow(rowIx);
            return;
        }
    }

    qWarning() << "❌ Nem található sor ezzel az entryId-vel:" << id;
}

void LeftoverTableManager::clear() {
    table->clearContents();
    table->setRowCount(0);
}

void LeftoverTableManager::refresh_TableFromRegistry() {
    if (!table)
        return;

    TableUtils::clearSafely(table);

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
/*
std::optional<CutResult> LeftoverTableManager::readRow(int row) const {
    if (!table || row < 0 || row >= table->rowCount())
        return std::nullopt;

    auto* itemMaterial = table->item(row, 1);
    auto* itemLength   = table->item(row, 2);
    auto* itemCuts     = table->item(row, 3);
    auto* itemWaste  = table->item(row, 4);
    auto* itemSource   = table->item(row, 5);

    if (!itemMaterial || !itemLength || !itemSource || !itemWaste)
        return std::nullopt;

    QUuid materialId = itemMaterial->data(Qt::UserRole).toUuid();

    if (materialId.isNull())
        return std::nullopt;

    int length = itemLength->data(Qt::UserRole).toInt();
    int waste = itemWaste->data(Qt::UserRole).toInt();

    int sourceVal = itemSource->data(Qt::UserRole).toInt();
    auto source = static_cast<LeftoverSource>(sourceVal);

    // Cuts lista lekérése — UserRole-ból (ha el van mentve)
    QVector<int> cuts;
    QVariant cutsData = itemCuts->data(Qt::UserRole);
    if (cutsData.canConvert<QVector<int>>())
        cuts = cutsData.value<QVector<int>>();

    return CutResult {
        materialId,
        length,
        cuts,
        waste,
        source,
        std::nullopt // ha van optimizationId, azt itt is be lehet tölteni
    };
}

*/
/*
std::optional<LeftoverStockEntry> LeftoverTableManager::readRow(int row) const {
    if (!table || row < 0 || row >= table->rowCount())
        return std::nullopt;

    auto* itemName   = table->item(row, ColName);
    auto* itemLength = table->item(row, ColLength);
    auto* itemReuId = table->item(row, ColReusableId);
    auto* itemSource   = table->item(row, ColSource); // 💡 ha a source is meg van

    if (!itemName || !itemLength || !itemReuId)
        return std::nullopt;

    QUuid materialId = itemName->data(Qt::UserRole).toUuid();
    int length = itemLength->data(Qt::UserRole).toInt();
    QString reusableId = itemReuId->text();

    if (materialId.isNull() || length <= 0)
        return std::nullopt;

    LeftoverSource source = LeftoverSource::Undefined;
    if (itemSource)
        source = static_cast<LeftoverSource>(itemSource->data(Qt::UserRole).toInt());

    if (materialId.isNull() || length <= 0 || reusableId.isEmpty())
        return std::nullopt;

    LeftoverStockEntry entry;

    entry.materialId = materialId;
    entry.availableLength_mm = length;
    entry.source = source;
    entry.optimizationId = std::nullopt;
    entry.barcode = reusableId;

    return entry;
}


QVector<LeftoverStockEntry> LeftoverTableManager::readAll() const {
    QVector<LeftoverStockEntry> results;

    int rowCount = table->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        auto maybe = readRow(row);
        if (maybe.has_value())
            results.append(*maybe);
    }

    return results;
}
*/


