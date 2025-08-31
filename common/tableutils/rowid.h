#pragma once

#include <QWidget>
#include <QUuid>
#include <QTableWidget>

class RowId{
private:
    static constexpr const char* _key="rowId";
    int _colIx;
    QTableWidget* _table;
public:
    RowId(QTableWidget* table, int colIx) {
        _table = table;
        _colIx = colIx;
    }

    void set(int rowIx, const QUuid& a){
        if(!_table) return;
        QWidget *widget = _table->cellWidget(rowIx, _colIx);
        if(!widget) return;
        widget->setProperty(_key, a);
    }

    QUuid get(int rowIx){
        if(!_table) return QUuid();
        QWidget *widget = _table->cellWidget(rowIx, _colIx);
        if(!widget) return QUuid();
        return widget->property(_key).toUuid();
    }

};
