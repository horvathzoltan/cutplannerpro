#pragma once

#include "common/logger.h"
#include <QToolBar>
#include <QAction>
#include <QStyle>
#include <QApplication>

namespace MainWindowUIBuilder {

inline QAction* createMaterialFinderAction(QObject* parent)
{
    if (!parent) {
        zWarning(L("No parent for action"));
        return nullptr;
    }

    QAction* act = new QAction("Material Finder", parent);
    act->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView));

    act->setObjectName("actionMaterialFinder");
    act->setToolTip("Find material in leftovers or stock");

    return act;
}

inline QAction* createSettingsAction(QObject* parent)
{
    QAction* act = new QAction(parent);

    act->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView));

    act->setText("Beállítások");
    act->setToolTip("Alkalmazás beállításai");

    return act;
}

}
