#pragma once

#include "common/logger.h"
#include <QToolBar>
#include <QAction>

namespace MainWindowUIBuilder {

inline QAction* createMaterialFinderAction(QObject* parent)
{
    if (!parent) {
        zWarning(L("No parent for action"));
        return nullptr;
    }

    QAction* act = new QAction("Material Finder", parent);
    act->setObjectName("actionMaterialFinder");
    act->setToolTip("Find material in leftovers or stock");

    return act;
}

}
