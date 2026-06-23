#pragma once

#include <QTableWidget>

#include <view/tableutils/RowTracker.h>
//#include "model/cutresult.h"
//#include "../tableutils/rowid.h"
#include "../../model/leftoverstockentry.h"

/// ♻️ Hullók (maradékok) táblájának kezelése  (vágás utáni maradékanyagok)
class LeftoverTableManager : public QObject {  // 🔧 QObject öröklés!
    Q_OBJECT                              // ✨ Qt metaobjektum makró!

public:
    explicit LeftoverTableManager(QTableWidget* table, QWidget* parent = nullptr);

    //QVector<LeftoverStockEntry> readAll() const;                 // 🔍 Beolvasás optimalizáláshoz
    //void clear();                                       // 🧹 Tábla ürítése

    //void fillTestData();
    void addRow(const LeftoverStockEntry &res);
    //void appendRows(const QVector<LeftoverStockEntry> &newResults);
    //std::optional<LeftoverStockEntry> readRow(int row) const;

signals:
    void deleteRequested(const QUuid& requestId);
    void editRequested(const QUuid& requestId);
    void editStorageRequested(const QUuid& requestId);

private:
    QTableWidget* table;
    QWidget* parent;  // 🔍 UI-hoz, hibajelzéshez, stb.
    //RowId _rowId;
    RowTracker _rows;
//    static constexpr int ReusableStockEntryIdIdRole = Qt::UserRole + 1;
    int _highlightedRow = -1;


public:
    static constexpr auto EntryId_Key = "entryId";

    // 🏷️ Oszlopindexek (UI: tableLeftovers)
    static constexpr int ColBarcode   = 0;
    static constexpr int ColMaterial      = 1;
    static constexpr int ColLength    = 2;

    static constexpr int ColStorageName  = 3;
    static constexpr int ColStatus     = 4;
    static constexpr int ColReusable  = 5;static constexpr int ColSource    = 6;

    static constexpr int ColCreatedAt   = 7;
    static constexpr int ColLastSeenAt   = 8;

    static constexpr int ColActions = 9;

    void refresh_TableFromRegistry();
    void updateRow(const LeftoverStockEntry &entry);
    void removeRowById(const QUuid &id);
    void highlight(const QUuid &id);
    void clearHighlight();
    void openScrapDialog();
};
