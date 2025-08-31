#include "inputtable_manager.h"
#include "common/tableutils/inputtable_rowstyler.h"
#include "common/tableutils/tableutils.h"
#include "model/registries/materialregistry.h"
//#include "common/tableutils/resulttable_rowstyler.h"
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QHBoxLayout>
#include <model/registries/cuttingplanrequestregistry.h>

InputTableManager::InputTableManager(QTableWidget* table, QWidget* parent)
    : QObject(parent), table(table), parent(parent),
    _rowId(table, ColName )
{}


void InputTableManager::addRow(const Cutting::Plan::Request& request) {
    if (!table)
        return;

    const MaterialMaster* mat= MaterialRegistry::instance().findById(request.materialId);
    if(!mat)
        return;

    int name_rowIx = table->rowCount();
    int data_rowIx = name_rowIx + 1;
    int meta_rowIx = name_rowIx + 2;
    table->insertRow(name_rowIx);           // Anyag név sor
    table->insertRow(data_rowIx);       // Fő adatsor
    table->insertRow(meta_rowIx);       // Meta adatsor


    //📛 Név + id
    table->setSpan(name_rowIx, ColMetaRowSpanStart, 1, 3);
    TableUtils::setMaterialNameCell(table, name_rowIx, ColName,
                                    mat->name,
                                    mat->color.color(),
                                    mat->color.name());

    _rowId.set(name_rowIx, mat->id);
    InputTable::RowStyler::applyStyle(table, name_rowIx, mat, request);

    // 📏 Hossz
    auto* itemLength = new QTableWidgetItem(QString::number(request.requiredLength));
    itemLength->setTextAlignment(Qt::AlignCenter);
    //itemLength->setData(Qt::UserRole, request.requiredLength);

    // 🔢 Mennyiség
    auto* itemQty = new QTableWidgetItem(QString::number(request.quantity));
    itemQty->setTextAlignment(Qt::AlignCenter);
   // itemQty->setData(Qt::UserRole, request.quantity);

    // 🗑️ Törlésgomb
    QPushButton* btnDelete = new QPushButton("🗑️");
    btnDelete->setToolTip("Sor törlése");
    btnDelete->setFixedSize(28, 28);
    btnDelete->setStyleSheet("QPushButton { border: none; }");
    btnDelete->setProperty(RequestId_Key, request.requestId);

    // ✏️ Update gomb
    QPushButton* btnUpdate = new QPushButton("✏️");
    btnUpdate->setToolTip("Sor szerkesztése");
    btnUpdate->setFixedSize(28, 28);
    btnUpdate->setStyleSheet("QPushButton { border: none; }");
    btnUpdate->setProperty(RequestId_Key, request.requestId);

    // 🎛️ Gombpanel
    auto* actionPanel = new QWidget();
    auto* layout = new QHBoxLayout(actionPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(btnUpdate);
    layout->addWidget(btnDelete);

    table->setCellWidget(data_rowIx, ColAction, actionPanel);
    table->setColumnWidth(ColAction, 64); // 2 gomb + spacing

    // 📡 Signal kapcsolás UUID alapú
    QObject::connect(btnDelete, &QPushButton::clicked, this, [btnDelete, this]() {
        QUuid id = btnDelete->property(RequestId_Key).toUuid();
        emit deleteRequested(id);
    });

    QObject::connect(btnUpdate, &QPushButton::clicked, this, [btnUpdate, this]() {
        QUuid id = btnUpdate->property(RequestId_Key).toUuid();
        emit editRequested(id);
    });


    // 📋 Fő adatsor beállítása
    table->setItem(data_rowIx, ColLength, itemLength);
    table->setItem(data_rowIx, ColQty, itemQty);

    InputTable::RowStyler::applyStyle(table, data_rowIx, mat, request);

    // 🧾 Alsó meta sor – 1 cella, 4 oszlopra kiterjesztve
    QLabel* metaLabel = new QLabel(
        QString("<span style='color:#555'>Megrendelő: <b>%1</b> &nbsp;&nbsp;•&nbsp;&nbsp; Tételszám: <i>%2</i></span>")
            .arg(request.ownerName, request.externalReference)
        );
    metaLabel->setStyleSheet("padding: 4px;");
    metaLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    table->setCellWidget(meta_rowIx, ColMetaRowSpanStart, metaLabel);
    table->setSpan(meta_rowIx, ColMetaRowSpanStart, 1, 3);
    table->setRowHeight(meta_rowIx, 24);
}

void InputTableManager::removeRowById(const QUuid& requestId) {
    for (int rowIx = 0; rowIx < table->rowCount(); ++rowIx) {
        QTableWidgetItem* item = table->item(rowIx, ColName);
        if (!item) continue;

        QUuid currentId = _rowId.get(rowIx);

        //QUuid id = item->data(CuttingRequestIdRole).toUuid();
        if (currentId == requestId) {
            table->removeRow(rowIx);     // fő sor
            table->removeRow(rowIx);
            table->removeRow(rowIx);     // meta sor is ugyanott
            return;
        }
    }
}

void InputTableManager::updateRow(const Cutting::Plan::Request& updated) {
    for (int name_rowIx = 0; name_rowIx < table->rowCount(); ++name_rowIx) {
        int data_rowIx = name_rowIx + 1;
        int meta_rowIx = name_rowIx + 2;

        QUuid currentId = _rowId.get(name_rowIx);
        if (currentId == updated.requestId) {
            const MaterialMaster* mat = MaterialRegistry::instance().findById(updated.materialId);
            if(!mat) continue;

            // 🔁 Anyag név frissítése
            TableUtils::setMaterialNameCell(table, name_rowIx, ColName,
                                            mat ? mat->name : "(ismeretlen)",
                                            mat ? mat->color.color() : QColor("#ccc"),
                                            mat ? mat->color.name() : "ismeretlen");

            InputTable::RowStyler::applyStyle(table, name_rowIx, mat, updated);

            // 🔁 Fő adatsor frissítése (row + 1)
            QTableWidgetItem* itemLength = table->item(data_rowIx, ColLength);
            if (itemLength) {
                itemLength->setText(QString::number(updated.requiredLength));
            }

            QTableWidgetItem* itemQty = table->item(data_rowIx, ColQty);
            if (itemQty) {
                itemQty->setText(QString::number(updated.quantity));
            }

            InputTable::RowStyler::applyStyle(table, data_rowIx, mat, updated);

            // 🔁 Frissítés a meta sorra is (feltételezzük, hogy mindig közvetlenül alatta van)
            QLabel* metaLabel = qobject_cast<QLabel*>(table->cellWidget(meta_rowIx, ColMetaRowSpanStart));
            if (metaLabel) {
                metaLabel->setText(
                    QString("<span style='color:#555'>Megrendelő: <b>%1</b> &nbsp;&nbsp;•&nbsp;&nbsp; Tételszám: <i>%2</i></span>")
                        .arg(updated.ownerName, updated.externalReference)
                    );
            }

            // 🎨 Stílus újraalkalmazása

            return;
        }
    }

    qWarning() << "⚠️ updateRow: Nem található sor a következő azonosítóval:" << updated.requestId;
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

void InputTableManager::clearTable() {
    if (!table)
        return;

    table->setRowCount(0);        // 💣 Teljes sorállomány törlése
    table->clearContents();       // 🧹 Cellák tartalmának kiürítése (nem kötelező, de biztosra megyünk)
}



