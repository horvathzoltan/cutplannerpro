#pragma once

#include <QTableWidget>
//#include "model/cutresult.h"
#include "model/reusablestockentry.h"

/// ♻️ Hullók (maradékok) táblájának kezelése  (vágás utáni maradékanyagok)
class LeftoverTableManager {
public:
    explicit LeftoverTableManager(QTableWidget* table, QWidget* parent = nullptr);

    QVector<ReusableStockEntry> readAll() const;                 // 🔍 Beolvasás optimalizáláshoz
    void clear();                                       // 🧹 Tábla ürítése

    void fillTestData();
    void addRow(const ReusableStockEntry &res);
    void appendRows(const QVector<ReusableStockEntry> &newResults);
    std::optional<ReusableStockEntry> readRow(int row) const;
private:
    QTableWidget* table;
    QWidget* parent;  // 🔍 UI-hoz, hibajelzéshez, stb.

    // 🏷️ Oszlopindexek (UI: tableLeftovers)
    static constexpr int ColName      = 0;
    static constexpr int ColBarcode   = 1;
    static constexpr int ColLength    = 2;
    static constexpr int ColShape     = 3;
    static constexpr int ColType      = 4;
    static constexpr int ColSource    = 5;
    static constexpr int ColReusable  = 6;
};
