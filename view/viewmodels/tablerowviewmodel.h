#pragma once

#include "tablecellviewmodel.h"
#include <QMap>
#include <QUuid>

struct TableRowViewModel {
    QUuid rowId = QUuid::createUuid(); // Egyedi azonosító (CRUD műveletekhez is kell);           // azonosító – interakcióhoz, mentéshez, csoporthoz
    QMap<int, TableCellViewModel> cells; // oszlopIndex → cellaViewModel
    bool isVisible = true;               // sor szintű láthatóság
};
