#include "leftovertablemanager.h"
#include "common/materialutils.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <common/rowstyler.h>
#include <model/registries/leftoverstockregistry.h>
#include "model/registries/materialregistry.h"

LeftoverTableManager::LeftoverTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent) {}

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

void LeftoverTableManager::addRow(const LeftoverStockEntry& entry) {
    if (!table)
        return;

    int row = table->rowCount();
    table->insertRow(row);

    const MaterialMaster* mat = entry.master();

    // 📛 Név
    auto* itemName = new QTableWidgetItem(mat ? mat->name : "(ismeretlen)");
    itemName->setTextAlignment(Qt::AlignCenter);
    itemName->setData(Qt::UserRole, entry.materialId);    
    itemName->setData(ReusableStockEntryIdIdRole, entry.entryId);
    table->setItem(row, ColName, itemName);

    // 🧾 Vonalkód
    auto* itemBarcode = new QTableWidgetItem(mat ? mat->barcode : "-");
    itemBarcode->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, ColBarcode, itemBarcode);

    auto* itemID = new QTableWidgetItem(entry.barcode);
    itemID->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, ColReusableId, itemID);

    // 📏 Hossz
    auto* itemLength = new QTableWidgetItem(QString::number(entry.availableLength_mm));
    itemLength->setTextAlignment(Qt::AlignCenter);
    itemLength->setData(Qt::UserRole, entry.availableLength_mm);
    table->setItem(row, ColLength, itemLength);

    // 📐 Forma
    auto* itemShape = new QTableWidgetItem(mat ? MaterialUtils::formatShapeText(*mat) : "-");
    itemShape->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, ColShape, itemShape);

    // // 🧬 Anyagtípus
    // auto* itemType = new QTableWidgetItem(mat ? mat->type.toString() : "-");
    // itemType->setTextAlignment(Qt::AlignCenter);
    // itemType->setData(Qt::UserRole, QVariant::fromValue(mat ? mat->type : MaterialType{}));
    // table->setItem(row, ColType, itemType);

    // 🛠️ Forrás
    auto* itemSource = new QTableWidgetItem(entry.sourceAsString());
    itemSource->setTextAlignment(Qt::AlignCenter);
    itemSource->setData(Qt::UserRole, static_cast<int>(entry.source));
    table->setItem(row, ColSource, itemSource);

    // ♻️ Újrahasználhatóság
    QString reuseMark = (entry.availableLength_mm >= 300) ? "✔" : "✘";
    auto* itemReusable = new QTableWidgetItem(reuseMark);
    itemReusable->setTextAlignment(Qt::AlignCenter);
    itemReusable->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
    itemReusable->setForeground(Qt::black);
    itemReusable->setData(Qt::UserRole, entry.availableLength_mm);
    table->setItem(row, ColReusable, itemReusable);

    // 🗑️ Törlés gomb
    QPushButton* btnDelete = new QPushButton("🗑️");
    btnDelete->setToolTip("Törlés");
    btnDelete->setFixedSize(28, 28);
    btnDelete->setStyleSheet("QPushButton { border: none; }");
    btnDelete->setProperty("barcode", entry.barcode);

    // ✏️ Szerkesztés gomb
    QPushButton* btnEdit = new QPushButton("✏️");
    btnEdit->setToolTip("Szerkesztés");
    btnEdit->setFixedSize(28, 28);
    btnEdit->setStyleSheet("QPushButton { border: none; }");
    btnEdit->setProperty("barcode", entry.barcode);

    // Panelbe helyezés
    auto* actionPanel = new QWidget();
    auto* layout = new QHBoxLayout(actionPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(btnEdit);
    layout->addWidget(btnDelete);

    table->setCellWidget(row, ColActions, actionPanel);
    table->setColumnWidth(ColActions, 64);

    // 📡 Signal kapcsolás UUID alapú
    QObject::connect(btnDelete, &QPushButton::clicked, this, [btnDelete, this]() {
        QUuid entryId = btnDelete->property("entryId").toUuid();
        emit deleteRequested(entryId);
    });

    QObject::connect(btnEdit, &QPushButton::clicked, this, [btnEdit, this]() {
        QUuid entryId = btnEdit->property("entryId").toUuid();
        emit editRequested(entryId);
    });

    // 🎨 Stílus
    RowStyler::applyReusableStyle(table, row, mat, entry);
}

void LeftoverTableManager::updateRow(const LeftoverStockEntry& entry) {
    if (!table)
        return;

    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* itemName = table->item(row, ColName);
        if (!itemName)
            continue;

        QUuid currentId = itemName->data(ReusableStockEntryIdIdRole).toUuid(); // 🔍 Saját role
        if (currentId == entry.entryId) {
            const MaterialMaster* mat = entry.master();

            QString materialName = mat ? mat->name : "(ismeretlen)";
            QString barcode = mat ? mat->barcode : "-";
            QString shape = mat ? MaterialUtils::formatShapeText(*mat) : "-";

            // 📛 Név
            itemName->setText(materialName);
            itemName->setData(Qt::UserRole, entry.materialId);
            itemName->setData(ReusableStockEntryIdIdRole, entry.entryId);

            // 🧾 Material vonalkód
            auto* itemBarcode = table->item(row, ColBarcode);
            if (itemBarcode) itemBarcode->setText(barcode);

            // 🧾 Maradék vonalkód
            auto* itemID = table->item(row, ColReusableId);
            if (itemID) itemID->setText(entry.barcode);

            // 📏 Hossz
            auto* itemLength = table->item(row, ColLength);
            if (itemLength) {
                itemLength->setText(QString::number(entry.availableLength_mm));
                itemLength->setData(Qt::UserRole, entry.availableLength_mm);
            }

            // 📐 Alak
            auto* itemShape = table->item(row, ColShape);
            if (itemShape) itemShape->setText(shape);

            // 🛠️ Forrás
            auto* itemSource = table->item(row, ColSource);
            if (itemSource) {
                itemSource->setText(entry.sourceAsString());
                itemSource->setData(Qt::UserRole, static_cast<int>(entry.source));
            }

            // ♻️ Újrahasználhatóság
            auto* itemReusable = table->item(row, ColReusable);
            if (itemReusable) {
                QString reuseMark = (entry.availableLength_mm >= 300) ? "✔" : "✘";
                itemReusable->setText(reuseMark);
                itemReusable->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
                itemReusable->setForeground(Qt::black);
                itemReusable->setData(Qt::UserRole, entry.availableLength_mm);
            }

            // 🎨 Sor stílus újraalkalmazása
            RowStyler::applyReusableStyle(table, row, mat, entry);

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
    for (int row = 0; row < table->rowCount(); ++row) {
        QVariant data = table->item(row, 0)->data(Qt::UserRole);
        if (data.canConvert<QUuid>() && data.toUuid() == id) {
            table->removeRow(row);
            return;
        }
    }

    qWarning() << "❌ Nem található sor ezzel az entryId-vel:" << id;
}

void LeftoverTableManager::clear() {
    table->clearContents();
    table->setRowCount(0);
}

void LeftoverTableManager::updateTableFromRegistry() {
    if (!table)
        return;

    table->clearContents();
    table->setRowCount(0);

    const auto& stockEntries = LeftoverStockRegistry::instance().all();
    const MaterialRegistry& materialReg = MaterialRegistry::instance();

    for (const auto& entry : stockEntries) {
        const MaterialMaster* master = materialReg.findById(entry.materialId);
        if (!master)
            continue;

        addRow(entry);
    }

    table->resizeColumnsToContents();
}




