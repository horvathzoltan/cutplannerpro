// view/viewmodels/tablecellviewmodel.h
#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

struct TableCellViewModel {


private:
    QWidget* _widget = nullptr; // opcionális, ha van interaktív komponens
    QString _text;
    QString _tooltip;
    QString _style; // opcionális egyedi stílus
    bool _isReadOnly = false; // alapértelmezett: csak olvasható
    bool _isVisible = true; // alapértelmezett: látható
    QColor _background;
    QColor _foreground;


public:
    void setStyleSheet(const QString& s){_style = s;}

    QString tooltip() const { return _tooltip;}
    void setToolTip(const QString& s){ _tooltip = s;}

    QString text() const { return _text;}

    bool hasWidget() const{ return _widget != nullptr;}
    QWidget* widget() const {return _widget;}
    void setWidget(QWidget* w) {_widget = w;}

    bool isReadOnly() const { return _isReadOnly;}
    bool isVisible() const { return _isVisible;}

    QColor background() const { return _background;}
    void setBackground(const QColor& c) { _background = c;}

    QColor foreground() const { return _foreground;}
    void setForeground(const QColor& c) { _foreground = c;}

    void apply() const{
        if(!_widget) return;

        _widget->setStyleSheet(_style);
        _widget->setToolTip(_tooltip);
    }

    static TableCellViewModel fromWidget(QWidget* widget,
                                         const QString& tooltip = QString(),
                                         bool isVisible = true) noexcept
    {
        TableCellViewModel vm;
        vm._widget = widget;
        vm._tooltip = tooltip;
        vm._isVisible = isVisible;
        vm._isReadOnly = true;
        vm._style = widget ? widget->styleSheet() : QString();
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
        vm._text = text;
        vm._tooltip = tooltip;
        vm._background = background;
        vm._foreground = foreground;
        vm._isReadOnly = isReadOnly;
        vm._isVisible = isVisible;
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
        vm._text = text;
        vm._tooltip = tooltip;
        vm._background = background;
        vm._foreground = foreground;
        vm._isReadOnly = isReadOnly;
        vm._isVisible = isVisible;
        vm._style = style;
        return vm;
    }
};
