#pragma once

#include "worksheetsummary/model/worksheetsummary.h"

#include <QWidget>

class WorkSheetSummaryDetailWidget
    : public QWidget
{
    Q_OBJECT

public:

    explicit WorkSheetSummaryDetailWidget(
        const WorkSheetSummary& summary,
        const QString& searchText = {},
        QWidget* parent = nullptr);
};