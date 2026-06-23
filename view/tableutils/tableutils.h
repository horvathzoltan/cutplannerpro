#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QUuid>

namespace TableUtils {

// 🆕 Biztonságos monospace font beállítása
// - Elsőként próbálja a Fira Code-ot
// - Ha nincs, akkor Consolas, Source Code Pro, Roboto Mono
// - Végül fallback: "monospace" (Qt a rendszer alap monospace fontját adja)
inline void applySafeMonospaceFont(QWidget* widget, int pointSize = 10) {
    QFont font;
    font.setFamilies({"Fira Code", "Consolas", "Source Code Pro", "Roboto Mono", "monospace"});
    font.setPointSize(pointSize);
    widget->setFont(font);
}

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
// inline QWidget* createStorageCell(const QString& storageName,
//                                   const QUuid& entryId,
//                                   QObject* receiver,
//                                   std::function<void()> onEdit)
// {
//     auto* panel = new QFrame();                     // 🔥 QWidget helyett QFrame
//     panel->setFrameStyle(QFrame::NoFrame);
//     panel->setAttribute(Qt::WA_NoSystemBackground); // 🔥 valódi átlátszóság
//     panel->setAutoFillBackground(false);

//     auto* layout = new QHBoxLayout(panel);
//     layout->setContentsMargins(0, 0, 0, 0);
//     layout->setSpacing(4);

//     auto* label = new QLabel(storageName);
//     label->setAlignment(Qt::AlignCenter);
//     label->setObjectName("lblStorageName");

//     // 🔥 A label is legyen átlátszó
//     label->setAttribute(Qt::WA_NoSystemBackground);
//     label->setAutoFillBackground(false);

//     auto* btnEdit = createIconButton("✏️", "Tároló módosítása", entryId);
//     btnEdit->setObjectName("btnEditStorage");

//     layout->addWidget(label);
//     layout->addWidget(btnEdit);

//     QObject::connect(btnEdit, &QPushButton::clicked, receiver, [entryId, onEdit]() {
//         onEdit();
//     });

//     return panel;
// }


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

// inline void updateStorageCell(QWidget* storagePanel,
//                               const QString& newStorageName,
//                               const QUuid& entryId)
// {
//     if (!storagePanel)
//         return;

//     // Label frissítése
//     if (auto* lblStorage = storagePanel->findChild<QLabel*>("lblStorageName")) {
//         lblStorage->setText(newStorageName);

//         // 🔥 Fontos: maradjon átlátszó!
//         lblStorage->setAttribute(Qt::WA_TranslucentBackground);
//         lblStorage->setAutoFillBackground(false);
//     } else {
//         qWarning() << "⚠️ QLabel 'lblStorageName' nem található a storagePanel-ben.";
//     }

//     // Edit gomb entryId frissítése
//     if (auto* btnEdit = storagePanel->findChild<QPushButton*>("btnEditStorage")) {
//         btnEdit->setProperty("entryId", entryId);
//     } else {
//         qWarning() << "⚠️ QPushButton 'btnEditStorage' nem található a storagePanel-ben.";
//     }

//     // 🔥 A panel háttérszínét SOHA nem állítjuk!
//     // A háttérszín mindig a QTableWidgetItem-en van.
//     storagePanel->setAttribute(Qt::WA_TranslucentBackground);
//     storagePanel->setAutoFillBackground(false);
// }

/*CommentCell*/
inline QWidget* createCommentCell(const QString& commentText, const QUuid& entryId, QObject* receiver, std::function<void()> onEdit) {
    auto* panel = new QWidget();
    auto* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* label = new QLabel(commentText);
    label->setWordWrap(true);
    label->setMinimumWidth(100);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* btnEdit = createIconButton("✏️", "Megjegyzés módosítása", entryId);

    layout->addWidget(label);
    layout->addWidget(btnEdit);

    label->setObjectName("lblComment");
    btnEdit->setObjectName("btnEditComment");

    QObject::connect(btnEdit, &QPushButton::clicked, receiver, [entryId, onEdit]() {
        onEdit();
    });

    return panel;
}

inline void updateCommentCell(QWidget* commentPanel, const QString& newComment, const QUuid& entryId) {
    if (!commentPanel)
        return;

    auto* lblComment = commentPanel->findChild<QLabel*>("lblComment");
    if (lblComment)
        lblComment->setText(newComment);
    else
        qWarning() << "⚠️ QLabel 'lblComment' nem található a commentPanel-ben.";

    auto* btnEdit = commentPanel->findChild<QPushButton*>("btnEditComment");
    if (btnEdit)
        btnEdit->setProperty("entryId", entryId);
    else
        qWarning() << "⚠️ QPushButton 'btnEditComment' nem található a commentPanel-ben.";
}

// struct CellDataModel {
//     int key;
//     QVariant value;
// };

/*MaterialNameCell*/
inline void setMaterialNameCell(QTableWidget* table, int row, int column,
                                const QString& name, const QColor& color, const QString& colorTooltip)
{
    if (!table)
        return;

    QLabel* nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignVCenter); // függőleges közép

    QWidget* namePanel = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(namePanel);
    layout->setContentsMargins(4, 0, 0, 0);     // kis bal margó
    layout->setSpacing(4);                      // szorosabb távolság
    layout->setAlignment(Qt::AlignLeft);        // balra igazítás

    layout->addWidget(nameLabel);               // először a név

    if (color.isValid()) {
        QLabel* colorBox = new QLabel();
        colorBox->setFixedSize(10, 10);         // diszkrét méret
        colorBox->setStyleSheet(QString("background-color: %1; border-radius: 5px; border: 1px solid #888;")
                                    .arg(color.name()));
        colorBox->setToolTip(colorTooltip);
        layout->addWidget(colorBox);            // utána a színminta
    }

    table->setCellWidget(row, column, namePanel);
}

inline void setReusableCell(QTableWidgetItem* item, int availableLength_mm) {
    if (!item)
        return;

    const bool reusable = availableLength_mm >= 300;
    const QString mark = reusable ? "✔" : "✘";
    const QColor bgColor = reusable ? QColor(144, 238, 144) : QColor(255, 200, 200);

    item->setText(mark);
    item->setTextAlignment(Qt::AlignCenter);
    item->setBackground(bgColor);
    item->setForeground(Qt::black);
    item->setData(Qt::UserRole, availableLength_mm); // opcionális, ha később kell
}

} //end namespace TableUtils
