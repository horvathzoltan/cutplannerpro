#pragma once
#include "model/material/material_utils.h"
#include "view/viewmodels/tablecellviewmodel.h"

#include <model/cutting/instruction/cutinstruction.h>

#include <QHBoxLayout>
#include <QLabel>

namespace CellGenerators {

// megcsinál egy materialcellt

// inline TableCellViewModel materialCell(const MaterialMaster& mat, const QString& barcode = "")
// {
//     QString text = MaterialUtils::materialToDisplay(mat,MaterialUtils::DisplayType::Label, barcode);
//     QString tooltip = MaterialUtils::materialToDisplay(mat,MaterialUtils::DisplayType::Tooltip, barcode);

//     QColor baseColor = GroupUtils::groupColor(mat.id);
//     QColor fgColor = baseColor.lightness() < 128 ? Qt::white : Qt::black;

//     return TableCellViewModel::fromText(text, tooltip, baseColor, fgColor);
// }

/**
 * @brief Anyag cella generátor, bogyóval kiegészítve.
 *
 * Feladata:
 *  - Az adott MaterialMaster objektumhoz tartozó szöveg és tooltip előállítása
 *    a MaterialUtils::materialToDisplay segítségével.
 *  - A szöveget két részre bontja: fő név (label1) és opcionális csoport/barcode rész (label2).
 *  - Meghatározza a csoporthoz tartozó háttérszínt (GroupUtils::groupColor),
 *    valamint a kontrasztos előtérszínt.
 *  - Létrehoz egy QWidget panelt, amely tartalmazza:
 *      • a fő név QLabel‑t,
 *      • opcionálisan egy kis színes „bogyót” (12×12 px), ha az anyaghoz van érvényes szín,
 *      • opcionálisan a második QLabel‑t (csoport/barcode rész).
 *  - A panel háttérszínét és előtérszínét a csoport színe alapján állítja be.
 *  - A cella tooltipje a MaterialUtils által generált részletes információ.
 *
 * @param mat     A MaterialMaster objektum, amely tartalmazza az anyag nevét, színét, barcode‑ját.
 * @param barcode Opcionális barcode string, amelyet a MaterialUtils::materialToDisplay felhasznál.
 *
 * @return TableCellViewModel, amely a felépített QWidget panelt tartalmazza,
 *         háttér és előtér színekkel kiegészítve.
 *
 * @note Ez a verzió vizuálisan gazdagabb, mint a sima fromText alapú cella:
 *       a név mellett megjeleníti az anyag színét egy kis kör formájában,
 *       így az operátor számára azonnali vizuális visszajelzést ad.
 */

inline TableCellViewModel materialCell(const MaterialMaster& mat, const QString& barcode = "")
{
    // 🏷️ Szöveg és tooltip előállítása
    QString text    = MaterialUtils::materialToDisplay(mat, MaterialUtils::DisplayType::Label,   barcode);
    QString tooltip = MaterialUtils::materialToDisplay(mat, MaterialUtils::DisplayType::Tooltip, barcode);
    QString label1,label2;

    int ix = text.indexOf(" (");
    if(ix >-1){
        label1 = text.left(ix);
        label2 = text.mid(ix);
    } else{
        label1 = text;
    }

    // 🎨 Csoport szín meghatározása
    QColor backgroundColor = GroupUtils::groupColor(mat.id);
    QColor foregroundColor = backgroundColor.lightness() < 128 ? Qt::white : Qt::black;

    // 🔹 Panel létrehozása a név + bogyó számára
    QWidget* panel = new QWidget();
    // panel->setContentsMargins(2, 2, 2, 2);
    // panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    // panel->setStyleSheet("margin:2px;");

    panel->setStyleSheet(QString("background-color: %1; color: %2;")
                             .arg(backgroundColor.name())
                             .arg(foregroundColor.name()));

    QHBoxLayout* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(4, 0, 0, 0);   // kis bal margó
    layout->setSpacing(4);                    // szorosabb távolság
    layout->setAlignment(Qt::AlignLeft);      // balra igazítás

    // 📛 Anyag név label
    QLabel* nameLabel1 = new QLabel(label1);
    nameLabel1->setAlignment(Qt::AlignVCenter); // függőleges közép

    //nameLabel1->setToolTip(tooltip);
    layout->addWidget(nameLabel1);

    // 🔵 Színes bogyó (ha van érvényes szín)
    if (mat.color.isValid()) {
        QColor m_fgColor = mat.color.color().lightness() < 128 ? Qt::white : Qt::black;

        QLabel* colorBox = new QLabel();
        colorBox->setAlignment(Qt::AlignVCenter); // függőleges közép

        colorBox->setFixedSize(12, 12); // diszkrét méret
        colorBox->setStyleSheet(QString(
                                    "background-color: %1; color: %2;"
                                    "border-radius: 5px; "
                                    "border: 1px solid #888;"
                                    ).arg(mat.color.color().name(),(m_fgColor.name())));
        colorBox->setToolTip(QString("Anyag színe: %1").arg(mat.color.name()));
        layout->addWidget(colorBox);
    }

    if(!label2.isEmpty()){
        QLabel* nameLabel2 = new QLabel(label2);
        nameLabel2->setAlignment(Qt::AlignVCenter); // függőleges közép
        //nameLabel2->setToolTip(tooltip);
        layout->addWidget(nameLabel2);
    }

    // 🔹 Layout hozzárendelése a panelhez
    panel->setLayout(layout);

    // ✅ Visszaadás widget formában
    auto r =  TableCellViewModel::fromWidget(panel, tooltip);
    r.background = backgroundColor;
    r.foreground = foregroundColor;
    return r;
}


} // namespace CellGenerators
