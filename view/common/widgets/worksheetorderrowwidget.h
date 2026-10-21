#pragma once

#include "worksheetsummary/model/worksheetorderinfo.h"

#include <QWidget>

class WorkSheetOrderRowWidget : public QWidget
{
    Q_OBJECT

public:

    explicit WorkSheetOrderRowWidget(
        const WorkSheetOrderInfo& order,
        const QString& searchText = {},
        QWidget* parent = nullptr);
};