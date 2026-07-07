#include "matrix_cell_delegate.h"

#include <QPainter>

void MatrixCellDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);   // lokális másolat
    painter->save();

    // 1) Háttérszín beállítása a StyleOptionViewItem-ben
    QVariant bgVar = index.data(Qt::BackgroundRole);
    if (bgVar.canConvert<QBrush>()) {
        opt.backgroundBrush = bgVar.value<QBrush>();
    }

    // 2) Alap cella kirajzolása
    QStyledItemDelegate::paint(painter, opt, index);

    // 3) Nem BOM cella áthúzása
    if (index.data(Qt::UserRole + 1).toString() == "nonBom") {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawLine(opt.rect.bottomLeft(), opt.rect.topRight());
    }

    // 4) Aktuális cella kerete
    if (index.data(Qt::UserRole).toString() == "active") {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(Qt::blue, 2));
        painter->drawRect(opt.rect.adjusted(1,1,-1,-1));
    }

    painter->restore();
}


// #include "matrix_cell_delegate.h"

// #include <QPainter>

// void MatrixCellDelegate::paint(QPainter* painter,
//                                const QStyleOptionViewItem& option,
//                                const QModelIndex& index) const
// {
//     QStyleOptionViewItem opt(option);   // lokális másolat
//     painter->save();

//     // 1) Háttérszín beállítása a StyleOptionViewItem-ben
//     QVariant bgVar = index.data(Qt::BackgroundRole);
//     if (bgVar.canConvert<QBrush>()) {
//         opt.backgroundBrush = bgVar.value<QBrush>();
//     }

//     // ⭐ 1/b) Aktív sor/oszlop highlight (háttérszín rétegben)
//     int activeRow = index.data(Qt::UserRole + 2).toInt();   // SeriesMatrixView beállítja
//     int activeCol = index.data(Qt::UserRole + 3).toInt();   // SeriesMatrixView beállítja

//     bool rowActive = (index.row() == activeRow);
//     bool colActive = (index.column() == activeCol);

//     // ⭐ Aktív sor/oszlop highlight (overlay rétegben)
//     if (rowActive || colActive) {
//         QColor hl(255, 255, 0, 40);
//         painter->drawRect(opt.rect.adjusted(2,2,-3,-3));
//     }


//     // 2) Alap cella kirajzolása
//     QStyledItemDelegate::paint(painter, opt, index);

//     // 3) Nem BOM cella áthúzása
//     if (index.data(Qt::UserRole + 1).toString() == "nonBom") {
//         painter->setPen(QPen(Qt::black, 1));
//         painter->drawLine(opt.rect.bottomLeft(), opt.rect.topRight());
//     }

//     // 4) Aktuális cella kerete
//     if (index.data(Qt::UserRole).toString() == "active") {
//         painter->setRenderHint(QPainter::Antialiasing, false);
//         QPen pen(Qt::blue);
//         pen.setWidth(2);
//         pen.setJoinStyle(Qt::MiterJoin);
//         painter->setPen(pen);
//         painter->drawRect(opt.rect.adjusted(2,2,-3,-3));

//     }

//     painter->restore();
// }


