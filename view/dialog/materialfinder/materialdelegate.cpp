#pragma once
#include "materialdelegate.h"

void MaterialDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();

    QVariant v = index.data(Qt::UserRole);
    if (!v.isValid()) {
        painter->restore();
        return;
    }

    MaterialMaster mat = v.value<MaterialMaster>();

    bool selected = option.state & QStyle::State_Selected;

    // 1️⃣ Háttérszín (combó currentIndex fallback)
    QColor bg;
    if (selected) {
        bg = option.palette.highlight().color();
    } else if (option.widget && option.widget->inherits("QComboBox")) {
        // combó current item esetén nincs selection flag → normál háttér
        bg = option.palette.base().color();
    } else {
        bg = GroupUtils::groupColor(mat.id);
    }

    // 2️⃣ Szöveg színe
    QColor fg = selected
                    ? option.palette.highlightedText().color()
                    : (bg.lightness() < 128 ? Qt::white : Qt::black);

    // 3️⃣ Háttér kirajzolása
    painter->fillRect(option.rect, bg);

    // 4️⃣ Belső margó
    QRect r = option.rect.adjusted(6, 2, -6, -2);

    // 5️⃣ Szöveg
    QString text = MaterialUtils::materialToDisplay(mat, DisplayType::Label);
    painter->setPen(fg);
    painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, text);

    // 6️⃣ Bogyó
    if (mat.color.isValid()) {

        // Antialiasing bekapcsolása
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Bogyó mérete
        const int size = 12;
        QRect dot(r.right() - size - 4, r.center().y() - size/2, size, size);

        // Kitöltés
        painter->setBrush(mat.color.color());

        // Border (ugyanaz, mint a CSS-ben)
        painter->setPen(QPen(QColor("#888"), 1));

        // Kör rajzolása
        painter->drawEllipse(dot);
    }


    painter->restore();
}



QSize MaterialDelegate::sizeHint(const QStyleOptionViewItem&,
                                 const QModelIndex&) const
{
    return QSize(220, 26);
}
