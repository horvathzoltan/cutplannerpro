#pragma once

#include "view/viewmodels/tablecellviewmodel.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUuid>
#include <QWidget>
#include "view/viewmodels/tablecellviewmodel.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QUuid>

namespace Relocation::ViewModel::CellGenerator {

TableCellViewModel createQuantityDialogButtonCell(const QUuid& rowId, QObject* receiver) {
    TableCellViewModel cell;

    auto* button = new QPushButton();
    button->setToolTip("Tárhelyek beállítása");
    button->setIcon(QIcon::fromTheme("preferences-system")); // vagy saját ikon: QIcon(":/icons/settings.png")
    button->setProperty("rowId", rowId);

    QObject::connect(button, &QPushButton::clicked, receiver, [button, receiver]() {
        QUuid rowId = button->property("rowId").toUuid();
        QMetaObject::invokeMethod(receiver, "editRow", Q_ARG(QUuid, rowId));
    });

    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(button);
    layout->setAlignment(Qt::AlignCenter);

    cell.widget = container;
    return cell;
}

/**
 * @brief Létrehoz egy cellát, amely tartalmazza a szöveget és egy ceruza gombot.
 * @param rowId   A sor azonosítója
 * @param text    A cellában megjelenő szöveg (pl. forrás/cél tartalom)
 * @param tooltip Tooltip a teljes tartalommal
 * @param receiver A táblamenedzser, amelynek van editRow(rowId, mode) slotja
 * @param mode    "source" vagy "target" – a dialógus fejléchez
 */
inline TableCellViewModel createEditableCell(const QUuid& rowId,
                                             const QString& text,
                                             const QString& tooltip,
                                             QObject* receiver,
                                             const QString& mode) {
    TableCellViewModel cell;

    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* label = new QLabel(text);
    label->setToolTip(tooltip);

    auto* button = new QPushButton();
    button->setIcon(QIcon::fromTheme("document-edit")); // ceruza ikon
    button->setToolTip(QString("Szerkesztés (%1)").arg(mode));
    button->setProperty("rowId", rowId);
    button->setProperty("mode", mode);

    QObject::connect(button, &QPushButton::clicked, receiver, [button, receiver]() {
        QUuid rowId = button->property("rowId").toUuid();
        QString mode = button->property("mode").toString();
        QMetaObject::invokeMethod(receiver, "editRow",
                                  Q_ARG(QUuid, rowId),
                                  Q_ARG(QString, mode));
    });

    layout->addWidget(label);
    layout->addWidget(button);
    layout->setAlignment(Qt::AlignLeft);

    cell.widget = container;
    return cell;
}

} // namespace Relocation::ViewModel::CellGenerator
