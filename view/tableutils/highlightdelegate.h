#pragma once
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSet>
#include <QAbstractItemView>

class HighlightDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit HighlightDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    int currentRowIx = -1;          // aktuális sor indexe
    QSet<int> completedRows;        // befejezett sor indexek

    void paint(QPainter* p, const QStyleOptionViewItem& opt,
               const QModelIndex& idx) const override {
        QStyleOptionViewItem option(opt);
        initStyleOption(&option, idx);

        // completed sor → szürke háttér
        if (completedRows.contains(idx.row())) {
            option.backgroundBrush = QBrush(QColor(220,220,220));
            option.palette.setColor(QPalette::Text, QColor(80,80,80));
        }

        // alap render
        QStyledItemDelegate::paint(p, option, idx);

        // aktuális sor → teljes sor kerete
        if (idx.row() == currentRowIx) {
            if (auto* view = qobject_cast<QAbstractItemView*>(parent())) {
                QRect rowRect = view->visualRect(idx.siblingAtColumn(0));
                rowRect.setWidth(view->viewport()->width());
                p->save();
                QPen pen(Qt::green, 2);
                p->setPen(pen);
                p->drawRect(rowRect.adjusted(0, 0, -1, -1));
                p->restore();
            }
        }
    }

    // delegate-ben:
    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override {
        QRect r = option.rect;
        if (index.row() == currentRowIx) {
            // csak az aktuális sorban kisebbítjük
            r = r.adjusted(0, 2, 0, -2);
        }
        editor->setGeometry(r);
    }
}; // endof HighlightDelegate
