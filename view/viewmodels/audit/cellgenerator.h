#pragma once

#include "common/logger.h"
#include "common/tableutils/storageaudittable_rowstyler.h"
#include "model/storageaudit/auditcontext_text.h"
#include "view/viewmodels/tablecellviewmodel.h"

#include "model/storageaudit/storageauditrow.h"

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
    if(_isVerbose){
        zInfo(L("[shouldShowAuditCheckbox] → %1 | módosítva: %2 | auditált: %3 | elvárt: %4 | egyezik: %5")
                  .arg(row.rowId.toString())
                  .arg(row.wasModified)
                  .arg(row.isAuditConfirmed)
                  .arg(row.pickingQuantity)
                  .arg(row.actualQuantity == row.originalQuantity));
    }
    // Csak akkor kell pipa, ha:
    // - nem módosították
    // - az érték megegyezik az eredetivel
    // - nincs még auditálva
    // - és van elvárt mennyiség (különben nincs mit auditálni)
    // return !row.wasModified
    //        && row.actualQuantity == row.originalQuantity
    //        && !row.isAuditConfirmed
    //        && row.pickingQuantity > 0;

    // return !row.wasModified
    //        && row.actualQuantity == row.originalQuantity
    //        && !row.isAuditConfirmed
    //        && (row.pickingQuantity > 0 || row.isInOptimization);

    // Csak akkor jelenjen meg a checkbox, ha a sor auditálható
    return row.pickingQuantity > 0 || row.isInOptimization;
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
        radioPresent->setChecked(row.presence == AuditPresence::Present);
        radioMissing->setChecked(row.presence == AuditPresence::Missing);


        QObject::connect(radioPresent, &QRadioButton::toggled, receiver, [radioPresent, receiver]() {
            if (radioPresent->isChecked()) {
                QUuid rowId = radioPresent->property("rowId").toUuid();
                QMetaObject::invokeMethod(receiver, "leftoverPresenceChanged",
                                          Q_ARG(QUuid, rowId),
                                          Q_ARG(AuditPresence, AuditPresence::Present));
            }
        });

        QObject::connect(radioMissing, &QRadioButton::toggled, receiver, [radioMissing, receiver]() {
            if (radioMissing->isChecked()) {
                QUuid rowId = radioMissing->property("rowId").toUuid();
                QMetaObject::invokeMethod(receiver, "leftoverPresenceChanged",
                                          Q_ARG(QUuid, rowId),
                                          Q_ARG(AuditPresence, AuditPresence::Missing));
            }
        });

        if (shouldShowAuditCheckbox(row)) {
            auto* checkbox = new QCheckBox("Auditálva");
            checkbox->setChecked(row.isAuditConfirmed);
            checkbox->setProperty("rowId", row.rowId);

            checkbox->setToolTip("Jelöld meg, ha az érték auditált, de nem módosult");
            checkbox->setStyleSheet(QString(
                                        "background-color: %1; color: %2;"
                                        "QToolTip { background-color: %1; color: %2; border: 1px solid gray; }"
                                        ).arg(cell.background.name()).arg(cell.foreground.name()));

            QObject::connect(checkbox, &QCheckBox::toggled, receiver, [checkbox, receiver](bool checked) {
                QUuid rowId = checkbox->property("rowId").toUuid();
                QMetaObject::invokeMethod(receiver, "auditCheckboxToggled",
                                          Q_ARG(QUuid, rowId),
                                          Q_ARG(bool, checked));
            });

            layout->addWidget(checkbox);
        }

        cell.widget = container;
    } else {
        // 🔢 SpinBox megoldás
        auto* spin = new QSpinBox();
        spin->setRange(0, 9999);
        spin->setValue(row.actualQuantity);
        spin->setProperty("rowId", row.rowId);
        // spin->setStyleSheet(QString("background-color: %1; color: %2;")
        //                         .arg(cell.background.name())
        //                         .arg(cell.foreground.name()));

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

        if (shouldShowAuditCheckbox(row)) {
            auto* checkbox = new QCheckBox("Auditálva");
            checkbox->setChecked(row.isAuditConfirmed);
            checkbox->setProperty("rowId", row.rowId);

            checkbox->setToolTip("Jelöld meg, ha az érték auditált, de nem módosult");
            // checkbox->setStyleSheet(QString(
            //                             "background-color: %1; color: %2;"
            //                             "QToolTip { background-color: %1; color: %2; border: 1px solid gray; }"
            //                             ).arg(cell.background.name()).arg(cell.foreground.name()));

            checkbox->setStyleSheet(QString(
                                        "background-color: %1; color: %2;"
                                        "QToolTip { background-color: #ffffcc; color: #000000; border: 1px solid gray; }"
                                        ).arg(cell.background.name()).arg(cell.foreground.name()));


            QObject::connect(checkbox, &QCheckBox::toggled, receiver, [checkbox, receiver](bool checked) {
                QUuid rowId = checkbox->property("rowId").toUuid();
                QMetaObject::invokeMethod(receiver, "auditCheckboxToggled",
                                          Q_ARG(QUuid, rowId),
                                          Q_ARG(bool, checked));
            });

            //            checkbox->setToolTip("Jelöld meg, ha az érték auditált, de nem módosult");
            // checkbox->setStyleSheet(QString("background-color: %1; color: %2;")
            //                         .arg(cell.background.name())
            //                         .arg(cell.foreground.name()));

            layout->addWidget(checkbox);
        }

        container->setStyleSheet(QString("background-color: %1; color: %2;")
                                     .arg(cell.background.name())
                                     .arg(cell.foreground.name()));

        cell.widget = container;

    }

    return cell;
}
}
