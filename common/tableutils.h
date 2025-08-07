#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QUuid>

namespace TableUtils {

inline void clearSafely(QTableWidget* table) {
    if (!table)
        return;

    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QWidget* widget = table->cellWidget(row, col);
            if (widget) {
                table->removeCellWidget(row, col);
                delete widget;
            }
        }
    }

    table->clearContents();
    table->setRowCount(0);
}

inline QPushButton* createIconButton(const QString& iconText, const QString& tooltip, const QUuid& entryId) {
    auto* btn = new QPushButton(iconText);
    btn->setToolTip(tooltip);
    btn->setFixedSize(28, 28);
    btn->setStyleSheet("QPushButton { border: none; }");
    btn->setProperty("entryId", entryId);
    return btn;
}

inline QWidget* createQuantityCell(int quantity, const QUuid& entryId, QObject* receiver, std::function<void()> onEdit) {
    auto* quantityPanel = new QWidget();
    auto* quantityLayout = new QHBoxLayout(quantityPanel);
    quantityLayout->setContentsMargins(0, 0, 0, 0);
    quantityLayout->setSpacing(4);

    auto* lblQty = new QLabel(QString::number(quantity));
    lblQty->setAlignment(Qt::AlignCenter);

    auto* btnEditQty = createIconButton("✏️", "Mennyiség módosítása", entryId);
    quantityLayout->addWidget(lblQty);
    quantityLayout->addWidget(btnEditQty);

    lblQty->setObjectName("lblQuantity");
    btnEditQty->setObjectName("btnEditQuantity");

    QObject::connect(btnEditQty, &QPushButton::clicked, receiver, [entryId, onEdit]() {
        onEdit();
    });

    return quantityPanel;
}

inline void updateQuantityCell(QWidget* quantityPanel, int newQuantity, const QUuid& entryId) {
    if (!quantityPanel)
        return;

    auto* lblQty = quantityPanel->findChild<QLabel*>("lblQuantity");
    auto* btnEdit = quantityPanel->findChild<QPushButton*>("btnEditQuantity");

    // 🔍 Megkeressük a QLabel-et
//    auto* lblQty = quantityPanel->findChild<QLabel*>();
    if (lblQty)
        lblQty->setText(QString::number(newQuantity));

    // 🔍 Megkeressük a gombot és frissítjük a property-t
  //  auto* btnEdit = quantityPanel->findChild<QPushButton*>();
    if (btnEdit)
        btnEdit->setProperty("entryId", entryId);
}

} //end namespace TableUtils
