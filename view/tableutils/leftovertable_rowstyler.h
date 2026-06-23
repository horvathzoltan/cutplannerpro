#pragma once

#include "materials/view/material_row_styler.h"
#include "../../model/leftoverstockentry.h"
#include "../managers/leftovertable_manager.h"
#include <model/registries/cuttingmachineregistry.h>
#include <model/machine/machineutils.h>

namespace LeftoverTable{
namespace RowStyler{

inline void applyStyle(QTableWidget* table, int row, const MaterialMaster* mat, const LeftoverStockEntry& entry) {
    if (!table || !mat)
        return;

    constexpr int ColReusable = LeftoverTableManager::ColReusable; // 🔁 Frissítsd, ha eltér;

    // 🎨 Kategóriaalapú háttérszín
    //QColor baseColor = MaterialUtils::colorForMaterial(*mat);

//     for (int col = 0; col < table->columnCount(); ++col) {
//         if (col == ColReusable)
//             continue;

//         // auto* item = table->item(row, col);
//         // if (!item) {
//         //     item = new QTableWidgetItem();
//         //     table->setItem(row, col, item);
//         // }

//         QString tip;

//         MaterialRowStyler::applyMaterialStyle(table, row, mat, tip);

// //        item->setBackground(baseColor);
//   //      item->setForeground(col == 0 ? Qt::white : Qt::black); // Név oszlop legyen világos betűs
//     }

    MaterialRowStyler::applyMaterialStyle(table, row, mat, {ColReusable});

    // ♻️ Újrahasználhatóság cella stílusa
    QString reuseMark = (entry.availableLength_mm >= 300) ? "✔" : "✘";
    auto* itemReuse = table->item(row, ColReusable);
    if (!itemReuse) {
        itemReuse = new QTableWidgetItem(reuseMark);
        table->setItem(row, ColReusable, itemReuse);
    } else {
        itemReuse->setText(reuseMark);
    }

    itemReuse->setTextAlignment(Qt::AlignCenter);
    itemReuse->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
    itemReuse->setForeground(Qt::black);


    // -----------------------------
    // Hibás prefix jelzése
    // -----------------------------
    {
        QString prefix = entry.barcode.left(3).toUpper();
        bool prefixOk = (prefix == "RSM" || prefix == "RST");

        if (!prefixOk) {
            if (auto* item = table->item(row, LeftoverTableManager::ColBarcode)) {
                item->setBackground(QColor(255, 220, 220)); // halvány piros
                item->setToolTip(QString(
                                     "⚠️ Hibás leftover kód: '%1'\n"
                                     "Csak RSM és RST prefix engedélyezett."
                                     ).arg(prefix));
            }
        }
    }



    // -----------------------------
    // 4) Gép-kompatibilitás ellenőrzése
    // -----------------------------
    {
        const CuttingMachine* machine =
            MachineUtils::findMachineForStorage(entry.storageId);

        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(entry.materialId);

        bool machineOk = MachineUtils::canMachineCutMaterial(machine, mat);

        if (!machineOk) {

            if (auto* w = table->cellWidget(row, LeftoverTableManager::ColStorageName)) {

                // háttér + fekete szöveg
                w->setStyleSheet("background-color: rgb(255, 245, 200); color: black;");

                QString machineName = machine ? machine->name : "—";
                QString matName = mat ? mat->name : "—";

                // Storage név
                const StorageEntry* storage =
                    StorageRegistry::instance().findById(entry.storageId);
                QString storageName = storage ? storage->name
                                              : entry.storageId.toString(QUuid::WithoutBraces);

                QStringList compat;
                if (machine) {
                    for (const auto& t : machine->compatibleMaterials)
                        compat << t.toString();
                }

                QString tooltip;

                if (!machine) {
                    // 🔥 NINCS gép → teljesen más üzenet
                    tooltip =
                        QString("⚠️ A leftover olyan tárolóban van, amely nincs géphez rendelve!\n\n"
                                "Anyag: %1\n"
                                "Tároló: %2\n"
                                "Gép: —")
                            .arg(matName)
                            .arg(storageName);
                } else {
                    // 🔥 Van gép, de nem kompatibilis
                    tooltip =
                        QString("⚠️ Rossz gépben tárolt leftover!\n\n"
                                "Anyag: %1\n"
                                "Tároló: %2\n"
                                "Gép: %3\n"
                                "Gép kompatibilis anyagai: %4")
                            .arg(matName)
                            .arg(storageName)
                            .arg(machineName)
                            .arg(compat.isEmpty() ? "—" : compat.join(", "));
                }

                w->setToolTip(tooltip);
            }
        }


    }

    // -----------------------------
    // 5) Audit aging – túl régen volt ellenőrizve
    // -----------------------------
    {
        QDateTime now = QDateTime::currentDateTime();
        qint64 days = entry.lastSeenAt.daysTo(now);

        if (auto* item = table->item(row, LeftoverTableManager::ColLastSeenAt)) {

            if (days > 90) {
                // 🔥 Kritikus: 90+ napja nem auditálták
                item->setBackground(QColor(255, 160, 160));   // erős, de nem rikító piros
                item->setToolTip(
                    QString("⚠️ Kritikus: %1 napja nem auditált leftover!\n"
                            "Utolsó audit: %2")
                        .arg(days)
                        .arg(entry.lastSeenAt.toString("yyyy-MM-dd HH:mm"))
                    );
            }
            else if (days > 30) {
                // ⚠️ Figyelmeztetés: 30–90 nap
                item->setBackground(QColor(255, 220, 170));   // narancs
                item->setToolTip(
                    QString("⚠️ Figyelmeztetés: %1 napja nem auditált leftover.\n"
                            "Utolsó audit: %2")
                        .arg(days)
                        .arg(entry.lastSeenAt.toString("yyyy-MM-dd HH:mm"))
                    );
            }
            else {
                // Friss audit → opcionálisan tooltip
                item->setToolTip(
                    QString("Utolsó audit: %1 (%2 napja)")
                        .arg(entry.lastSeenAt.toString("yyyy-MM-dd HH:mm"))
                        .arg(days)
                    );
            }
        }
    }


}

} // endof namespace RowStyler
} // endof namespace StockTable
