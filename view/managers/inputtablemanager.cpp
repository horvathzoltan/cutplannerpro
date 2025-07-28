#include "inputtablemanager.h"
#include "model/registries/materialregistry.h"
#include "common/rowstyler.h"
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QHBoxLayout>
#include <model/registries/cuttingrequestregistry.h>

InputTableManager::InputTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent) {}


void InputTableManager::addRow(const CuttingRequest& request) {
    int row = table->rowCount();
    table->insertRow(row);           // Fő adatsor
    table->insertRow(row + 1);       // Meta adatsor

    const auto opt = MaterialRegistry::instance().findById(request.materialId);
    const MaterialMaster* mat = opt ? &(*opt) : nullptr;

    // 🧪 Anyag név
    QString name = mat ? mat->name : "(ismeretlen)";
    auto* itemName = new QTableWidgetItem(name);
    itemName->setTextAlignment(Qt::AlignCenter);
    itemName->setData(Qt::UserRole, QVariant::fromValue(request.materialId));
    itemName->setData(CuttingRequestIdRole, request.requestId);

    // 📏 Hossz
    auto* itemLength = new QTableWidgetItem(QString::number(request.requiredLength));
    itemLength->setTextAlignment(Qt::AlignCenter);
    itemLength->setData(Qt::UserRole, request.requiredLength);

    // 🔢 Mennyiség
    auto* itemQty = new QTableWidgetItem(QString::number(request.quantity));
    itemQty->setTextAlignment(Qt::AlignCenter);
    itemQty->setData(Qt::UserRole, request.quantity);

    // 🗑️ Törlésgomb
    QPushButton* btnDelete = new QPushButton("🗑️");
    btnDelete->setToolTip("Sor törlése");
    btnDelete->setFixedSize(28, 28);
    btnDelete->setStyleSheet("QPushButton { border: none; }");

    // ✏️ Update gomb
    QPushButton* btnUpdate = new QPushButton("✏️");
    btnUpdate->setToolTip("Sor szerkesztése");
    btnUpdate->setFixedSize(28, 28);
    btnUpdate->setStyleSheet("QPushButton { border: none; }");

    // QObject::connect(btnDelete, &QPushButton::clicked, btnDelete, [this, btnDelete]() {
    //     QModelIndex index = this->table->indexAt(btnDelete->pos());
    //     int row = index.row();

    //     // 🔍 Lekérjük az itemName cellát és abból az azonosítót
    //     QTableWidgetItem* itemName = this->table->item(row, ColName);
    //     QUuid requestId = itemName->data(CuttingRequestIdRole).toUuid();

    //     // 🗑️ Törlés a táblából
    //     this->table->removeRow(row);     // fő sor
    //     this->table->removeRow(row);     // meta sor is lecsúszik, ugyanott

    //     // 🔄 Törlés a registryből
    //     CuttingRequestRegistry::instance().removeRequest(requestId);
    // });

    QObject::connect(btnDelete, &QPushButton::clicked, btnDelete, [this, btnDelete]() {
        QModelIndex index = this->table->indexAt(btnDelete->pos());
        int row = index.row();

        QTableWidgetItem* itemName = this->table->item(row, ColName);
        QUuid requestId = itemName->data(CuttingRequestIdRole).toUuid();

        emit deleteRequested(requestId);
    });

    QObject::connect(btnUpdate, &QPushButton::clicked, btnUpdate, [this, btnUpdate]() {
        QModelIndex index = this->table->indexAt(btnUpdate->pos());
        int row = index.row();

        QTableWidgetItem* itemName = this->table->item(row, ColName);
        QUuid requestId = itemName->data(CuttingRequestIdRole).toUuid();

        emit editRequested(requestId); // 🔔 Új signal
    });


    // 📋 Fő adatsor beállítása
    table->setItem(row, ColName, itemName);
    table->setItem(row, ColLength, itemLength);
    table->setItem(row, ColQty, itemQty);
    //table->setCellWidget(row, ColAction, btnDelete);

    // 🎛️ Gombpanel
    auto* btnPanel = new QWidget();
    auto* layout = new QHBoxLayout(btnPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // 🗑️ Delete gomb (már megvan)
    layout->addWidget(btnDelete);
    layout->addWidget(btnUpdate);

    table->setCellWidget(row, ColAction, btnPanel);
    table->setColumnWidth(ColAction, 64); // 2 gomb + spacing

    RowStyler::applyInputStyle(table, row, mat, request);

    // 🧾 Alsó meta sor – 1 cella, 4 oszlopra kiterjesztve
    QLabel* metaLabel = new QLabel(
        QString("<span style='color:#555'>Megrendelő: <b>%1</b> &nbsp;&nbsp;•&nbsp;&nbsp; Tételszám: <i>%2</i></span>")
            .arg(request.ownerName, request.externalReference)
        );
    metaLabel->setStyleSheet("padding: 4px;");
    metaLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    table->setCellWidget(row + 1, ColMetaRowSpanStart, metaLabel);
    table->setSpan(row + 1, ColMetaRowSpanStart, 1, 4);
    table->setRowHeight(row + 1, 24);
}

void InputTableManager::removeRowByRequestId(const QUuid& requestId) {
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* item = table->item(row, ColName);
        if (!item) continue;

        QUuid id = item->data(CuttingRequestIdRole).toUuid();
        if (id == requestId) {
            table->removeRow(row);     // fő sor
            table->removeRow(row);     // meta sor is ugyanott
            return;
        }
    }
}

void InputTableManager::updateTableFromRegistry() {
    if (!table)
        return;

    table->clearContents();
    table->setRowCount(0);

    const auto& requests = CuttingRequestRegistry::instance().readAll();
    for (const auto& req : requests) {
        addRow(req);  // ✅ feldolgozás és megjelenítés
    }

    table->resizeColumnsToContents();  // 📐 automatikus oszlopméretezés
}

void InputTableManager::updateRow(const CuttingRequest& updated) {
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* itemName = table->item(row, ColName);
        if (!itemName)
            continue;

        QUuid id = itemName->data(CuttingRequestIdRole).toUuid();
        if (id == updated.requestId) {
            const auto opt = MaterialRegistry::instance().findById(updated.materialId);
            const MaterialMaster* mat = opt ? &(*opt) : nullptr;
            QString materialName = mat ? mat->name : "(ismeretlen)";

            // 🔁 Frissített cellák
            itemName->setText(materialName);
            itemName->setData(Qt::UserRole, QVariant::fromValue(updated.materialId));
            itemName->setData(CuttingRequestIdRole, updated.requestId);

            QTableWidgetItem* itemLength = table->item(row, ColLength);
            if (itemLength) {
                itemLength->setText(QString::number(updated.requiredLength));
                itemLength->setData(Qt::UserRole, updated.requiredLength);
            }

            QTableWidgetItem* itemQty = table->item(row, ColQty);
            if (itemQty) {
                itemQty->setText(QString::number(updated.quantity));
                itemQty->setData(Qt::UserRole, updated.quantity);
            }

            // 🔁 Frissítés a meta sorra is (feltételezzük, hogy mindig közvetlenül alatta van)
            int metaRow = row + 1;
            QLabel* metaLabel = qobject_cast<QLabel*>(table->cellWidget(metaRow, ColMetaRowSpanStart));
            if (metaLabel) {
                metaLabel->setText(
                    QString("<span style='color:#555'>Megrendelő: <b>%1</b> &nbsp;&nbsp;•&nbsp;&nbsp; Tételszám: <i>%2</i></span>")
                        .arg(updated.ownerName, updated.externalReference)
                    );
            }

            // 🎨 Stílus újraalkalmazása
            RowStyler::applyInputStyle(table, row, mat, updated);

            return;
        }
    }

    qWarning() << "⚠️ updateRow: Nem található sor a következő azonosítóval:" << updated.requestId;
}


// void InputTableManager::addRow(const CuttingRequest& request) {
//     int row = table->rowCount();
//     table->insertRow(row);

//     const auto opt = MaterialRegistry::instance().findById(request.materialId);
//     const MaterialMaster* mat = opt ? &(*opt) : nullptr;

//     // 🧪 Anyag neve
//     QString name = mat ? mat->name : "(ismeretlen)";
//     auto* itemName = new QTableWidgetItem(name);
//     itemName->setTextAlignment(Qt::AlignCenter);
//     // ⬅️ id-itt tároljuk
//     itemName->setData(Qt::UserRole, QVariant::fromValue(request.materialId));

//     // 📏 Hossz
//     auto* itemLength = new QTableWidgetItem(QString::number(request.requiredLength));
//     itemLength->setTextAlignment(Qt::AlignCenter);
//     itemLength->setData(Qt::UserRole, request.requiredLength);

//     // 🔢 Mennyiség
//     auto* itemQty = new QTableWidgetItem(QString::number(request.quantity));
//     itemQty->setTextAlignment(Qt::AlignCenter);
//     itemQty->setData(Qt::UserRole, request.quantity);

//     table->setItem(row, 0, itemName);
//     table->setItem(row, 1, itemLength);
//     table->setItem(row, 2, itemQty);

//     // 🗑️ Törlésgomb
//     QPushButton* btnDelete = new QPushButton("🗑️");
//     btnDelete->setToolTip("Sor törlése");
//     btnDelete->setFixedSize(28, 28);
//     btnDelete->setStyleSheet("QPushButton { border: none; }");

//     QObject::connect(btnDelete, &QPushButton::clicked, btnDelete, [this, btnDelete]() {
//         int currentRow = this->table->indexAt(btnDelete->pos()).row();
//         this->table->removeRow(currentRow);
//     });

//     table->setCellWidget(row, 3, btnDelete);

//     RowStyler::applyInputStyle(table, row, mat, request);
// }


// std::optional<CuttingRequest> InputTableManager::readRow(int row) const {
//     auto* itemName   = table->item(row, ColName);
//     auto* itemLength = table->item(row, ColLength);
//     auto* itemQty    = table->item(row, ColQty);

//     QUuid materialId = itemName ? itemName->data(Qt::UserRole).toUuid() : QUuid();
//     int length       = itemLength ? itemLength->data(Qt::UserRole).toInt() : -1;
//     int quantity     = itemQty ? itemQty->data(Qt::UserRole).toInt() : -1;

//     CuttingRequest req;
//     req.materialId     = materialId;
//     req.requiredLength = length;
//     req.quantity       = quantity;

//     std::optional<CuttingRequest> result;
//     if (req.isValid())
//         result = req;

//     return result;
// }

// std::optional<CuttingRequest> InputTableManager::readRow(int row) const {
//     auto it = rowModelMap.find(row);
//     if (it != rowModelMap.end())
//         return it.value();
//     return std::nullopt;
// }


// QVector<CuttingRequest> InputTableManager::readAll() const {
//     return rowModelMap.values().toVector();
// }


// void InputTableManager::fillTestData() {
//     table->setRowCount(0);

//     const auto& materials = MaterialRegistry::instance().all();

//     if (materials.isEmpty()) {
//         QMessageBox::warning(parent, "Hiba", "Nincs anyag a törzsben.");
//         return;
//     }

//     if (materials.size() < 2) {
//         QMessageBox::warning(parent, "Hiba", "Legalább két különböző anyag szükséges a teszthez.");
//         return;
//     }

//     QVector<CuttingRequest> testRequests = {
//         { materials[0].id, 1800, 2, "Alfa Kft.", "EXT-001" },
//         { materials[1].id, 2200, 1, "Beta Zrt.", "EXT-002" },
//         { materials[0].id, 2900, 1, "Gamma Bt.", "EXT-003" }
//     };

//     for (const auto& req : testRequests) {
//         addRow(req);
//     }
// }
