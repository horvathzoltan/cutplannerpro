#pragma once

#include <QTableWidget>
//#include "model/cutresult.h"
#include "model/leftoverstockentry.h"

/// ♻️ Hullók (maradékok) táblájának kezelése  (vágás utáni maradékanyagok)
class LeftoverTableManager : public QObject {  // 🔧 QObject öröklés!
    Q_OBJECT                              // ✨ Qt metaobjektum makró!

public:
    explicit LeftoverTableManager(QTableWidget* table, QWidget* parent = nullptr);

    QVector<LeftoverStockEntry> readAll() const;                 // 🔍 Beolvasás optimalizáláshoz
    void clear();                                       // 🧹 Tábla ürítése

    //void fillTestData();
    void addRow(const LeftoverStockEntry &res);
    void appendRows(const QVector<LeftoverStockEntry> &newResults);
    std::optional<LeftoverStockEntry> readRow(int row) const;

signals:
    void deleteRequested(const QUuid& requestId);
    void editRequested(const QUuid& requestId);

private:
    QTableWidget* table;
    QWidget* parent;  // 🔍 UI-hoz, hibajelzéshez, stb.

    static constexpr int ReusableStockEntryIdIdRole = Qt::UserRole + 1;


public:
    // 🏷️ Oszlopindexek (UI: tableLeftovers)
    static constexpr int ColName      = 0;
    static constexpr int ColBarcode   = 1;
    static constexpr int ColReusableId   = 2;
    static constexpr int ColLength    = 3;
    static constexpr int ColShape     = 4;
    static constexpr int ColSource    = 5;
    static constexpr int ColReusable  = 6;
    static constexpr int ColActions = 7;

    void updateTableFromRegistry();
    void updateRow(const LeftoverStockEntry &entry);
    void removeRowById(const QUuid &id);
};
