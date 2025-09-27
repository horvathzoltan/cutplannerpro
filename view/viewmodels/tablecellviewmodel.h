// view/viewmodels/tablecellviewmodel.h
#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

struct TableCellViewModel {
    QString text;
    QString tooltip;
    QColor background;
    QColor foreground;
    QWidget* widget = nullptr; // opcionális, ha van interaktív komponens
    bool isReadOnly = false; // alapértelmezett: csak olvasható
    bool isVisible = true; // alapértelmezett: látható
};
