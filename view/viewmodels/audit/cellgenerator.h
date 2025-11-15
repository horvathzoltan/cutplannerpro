#pragma once

#include "../../../common/logger.h"
#include "../../tableutils/storageaudittable_rowstyler.h"
#include "../tablecellviewmodel.h"

#include "../../../model/storageaudit/storageauditrow.h"

#include <QSpinBox>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QUuid>
#include <QCheckBox>

namespace Audit::ViewModel::CellGenerator {

static const  bool _isVerbose = false;

inline bool shouldShowAuditCheckbox(const StorageAuditRow& row)
{
    if (_isVerbose) {
        zInfo(L("[shouldShowAuditCheckbox] → %1 | módosítva: %2 | auditált: %3 | elvárt: %4 | egyezik: %5")
                  .arg(row.rowId.toString())
                  .arg(row.isRowModified)
                  .arg(row.isRowAuditChecked)
                  .arg(row.totalExpected()) // már getterből
                  .arg(row.actualQuantity == row.originalQuantity));
    }

    if (row.sourceType == AuditSourceType::Leftover) {
        // Hullóknál: csak akkor kell checkbox, ha nincs módosítás (eredeti állapotban van)
        return !row.isRowModified;
    }

    // Stock soroknál marad az eredeti logika
    return !row.isRowModified && (
               row.totalExpected() > 0
               || row.isInOptimization
               );
}


inline QWidget* createAuditCheckboxWidget(const StorageAuditRow& row,
                                          QObject* receiver,
                                          const QColor& background,
                                          const QColor& foreground)
{
    constexpr int checkboxWidth = 80;

    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (shouldShowAuditCheckbox(row)) {
        auto* checkbox = new QCheckBox("Auditálva");
        checkbox->setChecked(row.isRowAuditChecked);
        checkbox->setProperty("rowId", row.rowId);

        checkbox->setToolTip("Jelöld meg, ha az érték auditált, de nem módosult");
        checkbox->setStyleSheet(QString(
                                    "background-color: %1; color: %2;"
                                    "QToolTip { background-color: #ffffcc; color: #000000; border: 1px solid gray; }"
                                    ).arg(background.name()).arg(foreground.name()));

        QObject::connect(checkbox, &QCheckBox::toggled, receiver, [checkbox, receiver](bool checked) {
            QUuid rowId = checkbox->property("rowId").toUuid();
            QMetaObject::invokeMethod(receiver, "auditCheckboxToggled",
                                      Q_ARG(QUuid, rowId),
                                      Q_ARG(bool, checked));
        });

        checkbox->setFixedWidth(checkboxWidth);
        layout->addWidget(checkbox);
    } else {
        // 🔹 Helyfoglaló widget, ha nincs checkbox
        auto* spacer = new QWidget();
        spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        spacer->setFixedWidth(checkboxWidth); // pl. 80
        layout->addWidget(spacer);

    }

    return container;
}



/// 🔹 Interaktív cella létrehozása az "actual" oszlophoz
/// Leftover esetén rádiógombokat, más esetben QSpinBox-ot használ
inline TableCellViewModel createActualCell(const StorageAuditRow& row,
                                           QObject* receiver,
                                           const QColor& background,
                                           const QColor& foreground = Qt::black)
{
    TableCellViewModel cell;
    cell.tooltip = QString("Tényleges mennyiség: %1").arg(row.actualQuantity);
    cell.background = background;
    cell.foreground = foreground;

    if (row.sourceType == AuditSourceType::Leftover) {
        // 🔘 Rádiógombos megoldás: "Van" / "Nincs"
        auto container = new QWidget();
        auto layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);

        auto radioPresent = new QRadioButton("Van");
        auto radioMissing = new QRadioButton("Nincs");

        layout->addWidget(radioPresent);
        layout->addWidget(radioMissing);

        radioPresent->setProperty("rowId", row.rowId);
        radioMissing->setProperty("rowId", row.rowId);
        radioPresent->setStyleSheet(QString("color: %1;").arg(cell.foreground.name()));
        radioMissing->setStyleSheet(QString("color: %1;").arg(cell.foreground.name()));
        container->setStyleSheet(QString("background-color: %1;").arg(cell.background.name()));

        auto rowFulfilled = row.isFulfilled();

        radioPresent->setChecked(rowFulfilled);//row.rowPresence == AuditPresence::Present);
        radioMissing->setChecked(!rowFulfilled);//row.rowPresence == AuditPresence::Missing);


        QObject::connect(radioPresent, &QRadioButton::toggled, receiver, [radioPresent, receiver]() {
            if (radioPresent->isChecked()) {
                QUuid rowId = radioPresent->property("rowId").toUuid();
                QMetaObject::invokeMethod(receiver, "leftoverQuantityChanged",
                                          Q_ARG(QUuid, rowId),
                                          Q_ARG(int, 1)); // "Van" → 1 db
            }
        });

        QObject::connect(radioMissing, &QRadioButton::toggled, receiver, [radioMissing, receiver]() {
            if (radioMissing->isChecked()) {
                QUuid rowId = radioMissing->property("rowId").toUuid();
                QMetaObject::invokeMethod(receiver, "leftoverQuantityChanged",
                                          Q_ARG(QUuid, rowId),
                                          Q_ARG(int, 0)); // "Nincs" → 0 db
            }
        });

        auto* checkbox = createAuditCheckboxWidget(row, receiver, cell.background, cell.foreground);
        layout->addWidget(checkbox);

        cell.widget = container;
    } else {
        // 🔢 SpinBox megoldás
        auto* spin = new QSpinBox();
        spin->setRange(0, 9999);
        spin->setValue(row.actualQuantity);
        spin->setProperty("rowId", row.rowId);

        spin->setStyleSheet(QString(
                                "background-color: %1; color: %2;"
                                "QToolTip { background-color: #ffffcc; color: #000000; border: 1px solid gray; }"
                                ).arg(cell.background.name()).arg(cell.foreground.name()));


        QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), receiver, [spin, receiver](int value) {
            QUuid rowId = spin->property("rowId").toUuid();
            QMetaObject::invokeMethod(receiver, "auditValueChanged",
                                      Q_ARG(QUuid, rowId),
                                      Q_ARG(int, value));
        });

        auto* container = new QWidget();
        auto* layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        layout->addWidget(spin);

        auto* checkbox = createAuditCheckboxWidget(row, receiver, cell.background, cell.foreground);
        layout->addWidget(checkbox);

        container->setStyleSheet(QString("background-color: %1; color: %2;")
                                     .arg(cell.background.name())
                                     .arg(cell.foreground.name()));

        cell.widget = container;

    }

    return cell;
}
}
