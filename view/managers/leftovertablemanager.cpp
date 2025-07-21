#include "leftovertablemanager.h"
#include "common/materialutils.h"
#include <QMessageBox>
#include <common/rowstyler.h>
#include <model/registries/reusablestockregistry.h>
#include "model/registries/materialregistry.h"

LeftoverTableManager::LeftoverTableManager(QTableWidget* table, QWidget* parent)
    : table(table), parent(parent)
{}
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

std::optional<ReusableStockEntry> LeftoverTableManager::readRow(int row) const {
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

    ReusableStockEntry entry;

    entry.materialId = materialId;
    entry.availableLength_mm = length;
    entry.source = source;
    entry.optimizationId = std::nullopt;
    entry.barcode = reusableId;

    return entry;
}


QVector<ReusableStockEntry> LeftoverTableManager::readAll() const {
    QVector<ReusableStockEntry> results;

    int rowCount = table->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        auto maybe = readRow(row);
        if (maybe.has_value())
            results.append(*maybe);
    }

    return results;
}

void LeftoverTableManager::addRow(const ReusableStockEntry& entry) {
    if (!table)
        return;

    int row = table->rowCount();
    table->insertRow(row);

    const MaterialMaster* mat = entry.master();

    // 📛 Név
    auto* itemName = new QTableWidgetItem(mat ? mat->name : "(ismeretlen)");
    itemName->setTextAlignment(Qt::AlignCenter);
    itemName->setData(Qt::UserRole, entry.materialId);
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

    // 🎨 Stílus
    RowStyler::applyReusableStyle(table, row, mat, entry);
}

/*
void LeftoverTableManager::addRow(const CutResult& res) {
    if (!table)
        return;

    int row = table->rowCount();
    table->insertRow(row);

    // ♻️ Újrahasználhatósági jelzés
    QString reuseMark = (res.waste >= 300) ? "✔" : "✘";
    auto* itemReusable = new QTableWidgetItem(reuseMark);
    itemReusable->setTextAlignment(Qt::AlignCenter);
    itemReusable->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
    itemReusable->setForeground(Qt::black);
    itemReusable->setData(Qt::UserRole, res.waste);

    // 📦 Anyagtörzs név és stílus
    const auto opt = MaterialRegistry::instance().findById(res.materialId);
    const MaterialMaster* master = opt ? &*opt : nullptr;

    // 🏷️ Oszlopindexek
    const int colIndex     = 0;
    const int colMaterial  = 1;
    const int colLength    = 2;
    const int colCuts      = 3;
    const int colReusable  = 4;
    const int colSource    = 5;

    // 🔧 Anyag + materialId mentése UserRole-ban
    auto* itemMaterial = new QTableWidgetItem(master ? master->name : "(ismeretlen)");
    itemMaterial->setTextAlignment(Qt::AlignCenter);
    itemMaterial->setData(Qt::UserRole, res.materialId);
    if (!master)
        itemMaterial->setToolTip("Ismeretlen anyag azonosító: " + res.materialId.toString());

    table->setItem(row, colMaterial, itemMaterial);


    // 🔢 Rúd index (1-től induló)
    auto* itemIndex = new QTableWidgetItem(QString::number(row + 1));
    itemIndex->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, colIndex, itemIndex);

    // 📏 Eredeti hossz
    auto* itemLength = new QTableWidgetItem(QString::number(res.length));
    itemLength->setTextAlignment(Qt::AlignCenter);
    itemLength->setData(Qt::UserRole, res.length);
    table->setItem(row, colLength, itemLength);

    // ✂️ Vágások listája (szöveg + UserRole)
    auto* itemCuts = new QTableWidgetItem(res.cutsAsString());
    itemCuts->setTextAlignment(Qt::AlignCenter);
    itemCuts->setData(Qt::UserRole, QVariant::fromValue(res.cuts));
    table->setItem(row, colCuts, itemCuts);

    // ♻️ Újrahasználhatóság cella
    table->setItem(row, colReusable, itemReusable);

    // 🛠️ Forrás cella (szöveg + UserRole)
    auto* itemSource = new QTableWidgetItem(res.sourceAsString());
    itemSource->setTextAlignment(Qt::AlignCenter);
    itemSource->setData(Qt::UserRole, static_cast<int>(res.source));
    table->setItem(row, colSource, itemSource);

    // 🎨 Stílus alkalmazása
    RowStyler::applyLeftoverStyle(table, row, master, res);
}
*/
void LeftoverTableManager::appendRows(const QVector<ReusableStockEntry>& newResults) {
    if (!table)
        return;

    for (const auto& e : newResults)
        addRow(e);
}


void LeftoverTableManager::clear() {
    table->clearContents();
    table->setRowCount(0);
}

void LeftoverTableManager::updateTableFromRepository() {
    if (!table)
        return;

    table->clearContents();
    table->setRowCount(0);

    const auto& stockEntries = ReusableStockRegistry::instance().all();
    const MaterialRegistry& materialReg = MaterialRegistry::instance();

    for (const auto& entry : stockEntries) {
        const MaterialMaster* master = materialReg.findById(entry.materialId);
        if (!master)
            continue;

        addRow(entry);
    }

    table->resizeColumnsToContents();
}


// void LeftoverTableManager::fillTestData() {
//     if (!table || !parent)
//         return;

//     const auto& materials = MaterialRegistry::instance().all();
//     if (materials.isEmpty()) {
//         QMessageBox::warning(parent, "Hiba", "Nincs anyag a törzsben.");
//         return;
//     }
//     if (materials.size() < 2) {
//         QMessageBox::warning(parent, "Hiba", "Legalább két különböző anyag szükséges a teszthez.");
//         return;
//     }

//     QVector<ReusableStockEntry> testLeftovers ={
//         {
//             { materials[0].id, 2940, LeftoverSource::Manual, std::nullopt, "RST-012" },
//             { materials[0].id, 180, LeftoverSource::Manual, std::nullopt, "RST-013" },
//             { materials[1].id, 100, LeftoverSource::Manual, std::nullopt, "RST-014" }
//         }
//     };

//     for (const auto& e : testLeftovers)
//         addRow(e);
// }

