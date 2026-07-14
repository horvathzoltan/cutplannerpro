#pragma once

#include "materials/view/material_row_styler.h"
#include "../../model/leftoverstockentry.h"
#include "../managers/leftovertable_manager.h"
#include "view/tableutils/colorlogicutils.h"
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

}

} // endof namespace RowStyler
} // endof namespace StockTable
