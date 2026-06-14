#pragma once
#include "materialdelegate.h"


void MaterialDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();

    // 1️⃣ Háttér kirajzolása (selection + normal)
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    else
        painter->fillRect(option.rect, option.palette.base());

    // 2️⃣ MaterialMaster lekérése
    QVariant v = index.data(Qt::UserRole);
    if (!v.isValid()) {
        painter->restore();
        return;
    }

    MaterialMaster mat = v.value<MaterialMaster>();

    // 3️⃣ Csoportszín + foreground
    QColor bg = GroupUtils::groupColor(mat.id);
    QColor fg = bg.lightness() < 128 ? Qt::white : Qt::black;

    // 4️⃣ Belső margó
    QRect r = option.rect.adjusted(6, 2, -6, -2);

    // 5️⃣ Anyag megjelenítendő szöveg
    QString text = MaterialUtils::materialToDisplay(mat, DisplayType::Label);

    painter->setPen(fg);
    painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, text);

    // 6️⃣ Bogyó (szín)
    if (mat.color.isValid()) {
        QRect dot(r.right() - 16, r.center().y() - 6, 12, 12);
        painter->setBrush(mat.color.color());
        painter->setPen(Qt::gray);
        painter->drawEllipse(dot);
    }

    painter->restore();
}

QSize MaterialDelegate::sizeHint(const QStyleOptionViewItem&,
                                 const QModelIndex&) const
{
    return QSize(220, 26);
}
