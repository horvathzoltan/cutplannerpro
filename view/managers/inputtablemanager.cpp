#include "inputtablemanager.h"
#include "model/materialregistry.h"
#include "common/rowstyler.h"
#include <QPushButton>
#include <QMessageBox>

InputTableManager::InputTableManager(QTableWidget* table, QWidget* parent)
    : table(table), parent(parent) {}

void InputTableManager::addRow(const CuttingRequest& request) {
    int row = table->rowCount();
    table->insertRow(row);

    const auto opt = MaterialRegistry::instance().findById(request.materialId);
    const MaterialMaster* mat = opt ? &(*opt) : nullptr;

    // 🧪 Anyag neve
    QString name = mat ? mat->name : "(ismeretlen)";
    auto* itemName = new QTableWidgetItem(name);
    itemName->setTextAlignment(Qt::AlignCenter);
    // ⬅️ id-itt tároljuk
    itemName->setData(Qt::UserRole, QVariant::fromValue(request.materialId));

    // 📏 Hossz
    auto* itemLength = new QTableWidgetItem(QString::number(request.requiredLength));
    itemLength->setTextAlignment(Qt::AlignCenter);
    itemLength->setData(Qt::UserRole, request.requiredLength);

    // 🔢 Mennyiség
    auto* itemQty = new QTableWidgetItem(QString::number(request.quantity));
    itemQty->setTextAlignment(Qt::AlignCenter);
    itemQty->setData(Qt::UserRole, request.quantity);

    table->setItem(row, 0, itemName);
    table->setItem(row, 1, itemLength);
    table->setItem(row, 2, itemQty);

    // 🗑️ Törlésgomb
    QPushButton* btnDelete = new QPushButton("🗑️");
    btnDelete->setToolTip("Sor törlése");
    btnDelete->setFixedSize(28, 28);
    btnDelete->setStyleSheet("QPushButton { border: none; }");

    QObject::connect(btnDelete, &QPushButton::clicked, btnDelete, [this, btnDelete]() {
        int currentRow = this->table->indexAt(btnDelete->pos()).row();
        this->table->removeRow(currentRow);
    });

    table->setCellWidget(row, 3, btnDelete);

    RowStyler::applyInputStyle(table, row, mat, request);
}

std::optional<CuttingRequest> InputTableManager::readRow(int row) const {
    auto* itemName   = table->item(row, 0);
    auto* itemLength = table->item(row, 1);
    auto* itemQty    = table->item(row, 2);

    QUuid materialId = itemName ? itemName->data(Qt::UserRole).toUuid() : QUuid();
    int length       = itemLength ? itemLength->data(Qt::UserRole).toInt() : -1;
    int quantity     = itemQty ? itemQty->data(Qt::UserRole).toInt() : -1;

    CuttingRequest req;
    req.materialId     = materialId;
    req.requiredLength = length;
    req.quantity       = quantity;

    if (!req.isValid())
        return std::nullopt;

    return req;
}

QVector<CuttingRequest> InputTableManager::readAll() const {
    QVector<CuttingRequest> result;

    for (int row = 0; row < table->rowCount(); ++row) {
        if (auto req = readRow(row); req && req->isValid())
            result.append(*req);
    }

    return result;
}

void InputTableManager::fillTestData() {
    table->setRowCount(0);

    const auto& materials = MaterialRegistry::instance().all();

    if (materials.isEmpty()) {
        QMessageBox::warning(parent, "Hiba", "Nincs anyag a törzsben.");
        return;
    }

    if (materials.size() < 2) {
        QMessageBox::warning(parent, "Hiba", "Legalább két különböző anyag szükséges a teszthez.");
        return;
    }

    QVector<CuttingRequest> testRequests = {
        { materials[0].id, 1800, 2 },
        { materials[1].id, 2200, 1 },
        { materials[0].id, 2900, 1 }
    };

    for (const auto& req : testRequests) {
        addRow(req);
    }
}
