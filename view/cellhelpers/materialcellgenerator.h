#pragma once
//#include "model/cutting/plan/request.h"
#include "model/material/material_utils.h"
#include "view/viewmodels/tablecellviewmodel.h"

#include <model/cutting/instruction/cutinstruction.h>

#include <QHBoxLayout>
#include <QLabel>

namespace CellGenerators {

inline TableCellViewModel materialCell(const MaterialMaster& mat, const QString& barcode = "")
{
    // 🏷️ Szöveg és tooltip előállítása
    QString text    = MaterialUtils::materialToDisplay(mat, DisplayType::Label,   barcode);
    QString tooltip = MaterialUtils::materialToDisplay(mat, DisplayType::Tooltip, barcode);
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
    //nameLabel1->setAlignment(Qt::AlignVCenter); // függőleges közép

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


// inline TableCellViewModel cuttingPlanRequestCell(const Cutting::Plan::Request& r, const QString& barcode = ""){
//     QString text    = MaterialUtils::materialToDisplay(mat, MaterialUtils::DisplayType::Label,   barcode);
//     QString tooltip = MaterialUtils::materialToDisplay(mat, MaterialUtils::DisplayType::Tooltip, barcode);

// }

inline TableCellViewModel requestColorCell(const NamedColor& requiredColor, const NamedColor& matColor)
{
    QString text = requiredColor.isValid() ? requiredColor.name() : "Nincs szín";
    QString tooltip = QString("Igényelt szín: %1").arg(text);

    // Festés jelzés, ha eltér az anyag színétől - illetve ha az anyag natúr, akkor is festeni kell
    bool isPaintingNeeded = requiredColor.isValid() && requiredColor.name() != matColor.name();
    if (isPaintingNeeded) {
        text += " 🎨";
        tooltip += "\n🎨 Festés szükséges";
    }

    QWidget* panel = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(4,0,0,0);
    layout->setSpacing(4);
    layout->setAlignment(Qt::AlignLeft);      // balra igazítás

    QLabel* nameLabel = new QLabel(text);
    layout->addWidget(nameLabel);

    if (requiredColor.isValid()) {
        QColor fgColor = requiredColor.color().lightness() < 128 ? Qt::white : Qt::black;
        QLabel* colorBox = new QLabel();
        //colorBox->setAlignment(Qt::AlignVCenter); // függőleges közép

        colorBox->setFixedSize(12,12);
        colorBox->setStyleSheet(QString(
                                    "background-color: %1; color: %2; "
                                    "border-radius: 5px; "
                                    "border: 1px solid #888;"
                                    ).arg(requiredColor.color().name(), fgColor.name()));
        layout->addWidget(colorBox);
    }

    panel->setLayout(layout);
    return TableCellViewModel::fromWidget(panel, tooltip);
}


} // namespace CellGenerators
