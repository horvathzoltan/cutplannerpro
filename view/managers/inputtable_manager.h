#pragma once

#include <QTableWidget>
//#include <optional>
//#include "view/tableutils/rowid.h"
#include "../../model/cutting/plan/request.h"

class InputTableManager: public QObject {  // 🔧 QObject öröklés!
    Q_OBJECT                              // ✨ Qt metaobjektum makró!

public:
    explicit InputTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const Cutting::Plan::Request& request);
    void removeRowById(const QUuid &requestId);

    void refresh_TableFromRegistry();

    void updateRow(const QUuid& rowId, const Cutting::Plan::Request& request); // ⬅️ új metódus

    void clearTable();
signals:
    void deleteRequested(const QUuid& requestId);
    void editRequested(const QUuid& requestId);

private:
    QTableWidget* _table;
    QWidget* parent;
    //RowId _rowId;

    QMap<QUuid, Cutting::Plan::Request> _rowMap;
    // 👉 gyors lookup: rowId → rowIndex (így nem kell végigiterálni a táblát update-nél)
    QMap<QUuid, int> _rowIndexMap;

    static bool _isVerbose; // 👉 debug logging flag


};


