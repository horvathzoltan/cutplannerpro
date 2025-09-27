#pragma once

#include "tablecellviewmodel.h"
#include <QMap>
#include <QUuid>

struct TableRowViewModel {
    QUuid rowId;           // azonosító – interakcióhoz, mentéshez, csoporthoz
    QMap<int, TableCellViewModel> cells; // oszlopIndex → cellaViewModel
    bool isVisible = true;               // sor szintű láthatóság
};
