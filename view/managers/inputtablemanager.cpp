#include "inputtablemanager.h"
#include "common/tableutils/tableutils.h"
#include "model/registries/materialregistry.h"
#include "common/rowstyler.h"
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QHBoxLayout>
#include <model/registries/cuttingplanrequestregistry.h>

InputTableManager::InputTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent) {}


void InputTableManager::addRow(const CuttingPlanRequest& request) {
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
    btnDelete->setProperty("entryId", request.requestId);

    // ✏️ Update gomb
    QPushButton* btnUpdate = new QPushButton("✏️");
    btnUpdate->setToolTip("Sor szerkesztése");
    btnUpdate->setFixedSize(28, 28);
    btnUpdate->setStyleSheet("QPushButton { border: none; }");
    btnUpdate->setProperty("entryId", request.requestId);

    // 🎛️ Gombpanel
    auto* actionPanel = new QWidget();
    auto* layout = new QHBoxLayout(actionPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(btnUpdate);
    layout->addWidget(btnDelete);

    table->setCellWidget(row, ColAction, actionPanel);
    table->setColumnWidth(ColAction, 64); // 2 gomb + spacing

    // 📡 Signal kapcsolás UUID alapú
    QObject::connect(btnDelete, &QPushButton::clicked, this, [btnDelete, this]() {
        QUuid id = btnDelete->property("entryId").toUuid();
        emit deleteRequested(id);
    });

    QObject::connect(btnUpdate, &QPushButton::clicked, this, [btnUpdate, this]() {
        QUuid id = btnUpdate->property("entryId").toUuid();
        emit editRequested(id);
    });


    // 📋 Fő adatsor beállítása
    table->setItem(row, ColName, itemName);
    table->setItem(row, ColLength, itemLength);
    table->setItem(row, ColQty, itemQty);
    //table->setCellWidget(row, ColAction, btnDelete);

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

void InputTableManager::removeRowById(const QUuid& requestId) {
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

void InputTableManager::refresh_TableFromRegistry() {
    if (!table)
        return;

    TableUtils::clearSafely(table);

    const auto& requests = CuttingPlanRequestRegistry::instance().readAll();
    for (const auto& req : requests) {
        addRow(req);  // ✅ feldolgozás és megjelenítés
    }

    //table->resizeColumnsToContents();  // 📐 automatikus oszlopméretezés
}

void InputTableManager::updateRow(const CuttingPlanRequest& updated) {
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

void InputTableManager::clearTable() {
    if (!table)
        return;

    table->setRowCount(0);        // 💣 Teljes sorállomány törlése
    table->clearContents();       // 🧹 Cellák tartalmának kiürítése (nem kötelező, de biztosra megyünk)
}



