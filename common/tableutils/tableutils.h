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

/*QuantityCell*/
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

    // 🔍 Mennyiség label frissítése
    auto* lblQty = quantityPanel->findChild<QLabel*>("lblQuantity");
    if (lblQty)
        lblQty->setText(QString::number(newQuantity));
    else
        qWarning() << "⚠️ QLabel 'lblQuantity' nem található a quantityPanel-ben.";

    // 🔍 Gomb property frissítése
    auto* btnEdit = quantityPanel->findChild<QPushButton*>("btnEditQuantity");
    if (btnEdit)
        btnEdit->setProperty("entryId", entryId);
    else
        qWarning() << "⚠️ QPushButton 'btnEditQuantity' nem található a quantityPanel-ben.";
}

/*StorageCell*/
inline QWidget* createStorageCell(const QString& storageName, const QUuid& entryId, QObject* receiver, std::function<void()> onEdit) {
    auto* panel = new QWidget();
    auto* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* label = new QLabel(storageName);
    label->setAlignment(Qt::AlignCenter);

    auto* btnEdit = createIconButton("✏️", "Tároló módosítása", entryId);

    layout->addWidget(label);
    layout->addWidget(btnEdit);

    label->setObjectName("lblStorageName");
    btnEdit->setObjectName("btnEditStorage");

    QObject::connect(btnEdit, &QPushButton::clicked, receiver, [entryId, onEdit]() {
        onEdit();
    });

    return panel;
}

inline void updateStorageCell(QWidget* storagePanel, const QString& newStorageName, const QUuid& entryId) {
    if (!storagePanel)
        return;

    auto* lblStorage = storagePanel->findChild<QLabel*>("lblStorageName");
    if (lblStorage)
        lblStorage->setText(newStorageName);
    else
        qWarning() << "⚠️ QLabel 'lblStorageName' nem található a storagePanel-ben.";

    auto* btnEdit = storagePanel->findChild<QPushButton*>("btnEditStorage");
    if (btnEdit)
        btnEdit->setProperty("entryId", entryId);
    else
        qWarning() << "⚠️ QPushButton 'btnEditStorage' nem található a storagePanel-ben.";
}


} //end namespace TableUtils
