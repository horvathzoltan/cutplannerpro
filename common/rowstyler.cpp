#include "rowstyler.h"
#include "common/materialutils.h"
#include "model/reusablestockentry.h"
#include <QColor>
#include <QTableWidgetItem>

void RowStyler::applyInputStyle(QTableWidget* table, int row,
                                const MaterialMaster* mat,
                                const CuttingRequest& request) {
    if (!mat) return;

    QColor bg = GroupUtils::colorForGroup(mat->id);
    QString groupName = GroupUtils::groupName(mat->id);

    QString baseTip = QString("Anyag: %1\nCsoport: %2\nBarcode: %3")
                          .arg(mat->name)
                          .arg(groupName.isEmpty() ? "Nincs csoport" : groupName)
                          .arg(mat->barcode);

    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* item = table->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            table->setItem(row, col, item);
        }

        item->setBackground(bg);
        item->setForeground(bg.lightness() < 128 ? Qt::white : Qt::black);
        item->setTextAlignment(Qt::AlignCenter);

        switch (col) {
        case 0:
            item->setToolTip(baseTip);
            break;
        case 1:
            item->setToolTip(QString("Vágáshossz: %1 mm\n%2").arg(request.requiredLength).arg(baseTip));
            break;
        case 2:
            item->setToolTip(QString("Darabszám: %1 db\n%2").arg(request.quantity).arg(baseTip));
            break;
        default:
            break;
        }
    }
}

void RowStyler::applyStockStyle(QTableWidget* table, int row, const MaterialMaster* mat) {
    if (!table || !mat)
        return; // 🚫 Ha nincs érvényes táblázat vagy anyag, kilépünk

    QColor bgColor;
    QColor textColor = Qt::black;

    // 🎨 Háttérszín beállítása a hossz alapján — vizuális kategóriák
    if (mat->stockLength_mm <= 1000)
        bgColor = QColor("#fdd");       // 🌕 Rövid (pirosas háttér)
    else if (mat->stockLength_mm <= 2500)
        bgColor = QColor("#ffeeba");    // 🌼 Közepes (sárgás háttér)
    else
        bgColor = QColor("#d4edda");    // 🍃 Hosszú (zöldes háttér)

    // 🎯 Stílus alkalmazása minden cellára a sorban
    for (int col = 0; col < table->columnCount(); ++col) {
        QTableWidgetItem* item = table->item(row, col);
        if (!item) {
            item = new QTableWidgetItem;
            table->setItem(row, col, item); // 🧱 Hiányzó cella létrehozása
        }
        item->setBackground(bgColor);
        item->setForeground(textColor);
    }

    // 📝 Tooltip hozzáadása a "Hossz" mezőhöz (például a 3. oszlophoz, index 2)
    QTableWidgetItem* tooltipItem = table->item(row, 2);
    if (!tooltipItem) {
        tooltipItem = new QTableWidgetItem;
        table->setItem(row, 2, tooltipItem);
    }
    tooltipItem->setToolTip(QString("Hossz: %1 mm").arg(mat->stockLength_mm));
}


// void RowStyler::applyLeftoverStyle(QTableWidget* table, int row, const MaterialMaster* master, const CutResult& res) {
//     if (!table || !master)
//         return;

//     const int colReusable = 4;

//     // 🎨 Kategóriaalapú háttérszín az anyag szerint
//     QColor baseColor = MaterialUtils::colorForMaterial(*master);

//     // 🗂️ Végigmegyünk az sorcellákon
//     for (int col = 0; col < table->columnCount(); ++col) {
//         if (col == colReusable)
//             continue; // a "✔/✘" oszlop ne kapjon anyagszínt

//         auto* item = table->item(row, col);
//         if (!item) {
//             item = new QTableWidgetItem();
//             table->setItem(row, col, item);
//         }

//         item->setBackground(baseColor);
//         item->setForeground(col == 1 ? Qt::white : Qt::black); // név cella világosabb
//     }

//     // 🧃 Újrahasznosíthatóság stílusa – csak a 4. oszlopban
//     QString reuseMark = (res.waste >= 300) ? "✔" : "✘";
//     auto* reusableItem = new QTableWidgetItem(reuseMark);
//     reusableItem->setTextAlignment(Qt::AlignCenter);
//     reusableItem->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
//     reusableItem->setForeground(Qt::black);
//     table->setItem(row, colReusable, reusableItem);
// }

void RowStyler::applyReusableStyle(QTableWidget* table, int row, const MaterialMaster* master, const ReusableStockEntry& entry) {
    if (!table || !master)
        return;

    constexpr int ColReusable = 6;

    // 🎨 Kategóriaalapú háttérszín
    QColor baseColor = MaterialUtils::colorForMaterial(*master);

    for (int col = 0; col < table->columnCount(); ++col) {
        if (col == ColReusable)
            continue;

        auto* item = table->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            table->setItem(row, col, item);
        }

        item->setBackground(baseColor);
        item->setForeground(col == 0 ? Qt::white : Qt::black); // Név oszlop legyen világos betűs
    }

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
}

