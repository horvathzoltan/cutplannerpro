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
    QString style; // opcionális egyedi stílus
    bool isReadOnly = false; // alapértelmezett: csak olvasható
    bool isVisible = true; // alapértelmezett: látható

    static TableCellViewModel fromWidget(QWidget* widget,
                                         const QString& tooltip = QString(),
                                         bool isVisible = true) noexcept
    {
        TableCellViewModel vm;
        vm.widget = widget;
        vm.tooltip = tooltip;
        vm.isVisible = isVisible;
        vm.isReadOnly = true;
        return vm;
    }

    static TableCellViewModel fromText(const QString& text,
                                       const QString& tooltip = QString(),
                                       const QColor& background = Qt::white,
                                       const QColor& foreground = Qt::black,
                                       bool isReadOnly = true,
                                       bool isVisible = true) noexcept
    {
        TableCellViewModel vm;
        vm.text = text;
        vm.tooltip = tooltip;
        vm.background = background;
        vm.foreground = foreground;
        vm.isReadOnly = isReadOnly;
        vm.isVisible = isVisible;
        return vm;
    }

    static TableCellViewModel fromStyledText(const QString& text,
                                       const QString& tooltip = QString(),
                                       const QColor& background = Qt::white,
                                       const QColor& foreground = Qt::black,
                                       const QString& style = QString(),
                                       bool isReadOnly = true,
                                       bool isVisible = true) noexcept
    {
        TableCellViewModel vm;
        vm.text = text;
        vm.tooltip = tooltip;
        vm.background = background;
        vm.foreground = foreground;
        vm.isReadOnly = isReadOnly;
        vm.isVisible = isVisible;
        vm.style = style;
        return vm;
    }
};
