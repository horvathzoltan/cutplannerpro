#pragma once
#include "view/viewmodels/tablerowviewmodel.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

namespace TableRowPopulator {

/**
 * @brief Egy teljes sor feltöltése a TableRowViewModel alapján.
 *
 * A ViewModel minden cellája tartalmazza:
 * - szöveget (`text`)
 * - tooltipet (`tooltip`)
 * - háttér- és előtérszínt (`background`, `foreground`)
 * - interaktív widgetet (`widget`), ha van
 *
 * Ez a függvény gondoskodik arról, hogy a QTableWidget megfelelően jelenítse meg
 * a cellákat – legyenek azok sima szöveges itemek vagy interaktív widgetek.
 *
 * @param table A cél QTableWidget
 * @param rowIx A sor indexe, ahová a cellákat be kell helyezni
 * @param vm A TableRowViewModel, amely tartalmazza a cellák definícióját
 */
inline void populateRow(QTableWidget* table, int rowIx, const TableRowViewModel& vm) {
    for (auto it = vm.cells.constBegin(); it != vm.cells.constEnd(); ++it) {
        int col = it.key();
        const TableCellViewModel& cell = it.value();

        // widget eset - QWidget
        if (cell.widget) {
            // 🔘 Interaktív cella (pl. SpinBox, RadioButton)
            table->setCellWidget(rowIx, col, cell.widget);
            if(!cell.tooltip.isEmpty()){
                cell.widget->setToolTip(cell.tooltip);
            }
        }
        // cellatartalom eset - QTableWidgetItem
        else
        {
            // 🧾 Sima szöveges cella
            auto* item = new QTableWidgetItem(cell.text);
            if(!cell.tooltip.isEmpty()){
                item->setToolTip(cell.tooltip);
                }
            item->setBackground(cell.background);
            item->setForeground(cell.foreground);

            // 🔒 Szerkeszthetőség beállítása
            auto flags = item->flags();
            if (cell.isReadOnly)
                item->setFlags(flags & ~Qt::ItemIsEditable);
            else
                item->setFlags(flags | Qt::ItemIsEditable);

            table->setItem(rowIx, col, item);
        }
    }
}

} // namespace TableRowPopulator
