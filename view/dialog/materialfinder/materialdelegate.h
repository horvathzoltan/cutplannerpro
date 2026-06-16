#pragma once

#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>

#include "materials/model/material_master.h"
#include "materials/utils/material_utils.h"
//#include "materials/utils/group_utils.h"

Q_DECLARE_METATYPE(MaterialMaster)

class MaterialDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MaterialDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};
