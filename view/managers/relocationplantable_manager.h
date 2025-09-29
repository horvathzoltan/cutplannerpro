#pragma once

#include <QObject>
#include <QTableWidget>
#include <QMap>
#include <QUuid>
#include "model/relocation/relocationinstruction.h"

class RelocationPlanTableManager : public QObject {
    Q_OBJECT

private:
    QTableWidget* _table;   // 👉 the actual QTableWidget instance
    QWidget* _parent;       // 👉 parent widget, ha kell ownership chain

    // 👉 belső storage: rowId → RelocationInstruction
    QMap<QUuid, RelocationInstruction> _planRowMap;
    // 👉 gyors lookup: rowId → rowIndex (így nem kell végigiterálni a táblát update-nél)
    QMap<QUuid, int> _rowIndexMap;

    static bool _isVerbose; // 👉 debug logging flag

public:
    explicit RelocationPlanTableManager(QTableWidget* table, QWidget* parent = nullptr);

    // 👉 új sor beszúrása
    void addRow(const RelocationInstruction& instr);
    // 👉 meglévő sor frissítése rowId alapján
    void updateRow(const QUuid& rowId, const RelocationInstruction& instr);
    // 👉 teljes tábla törlése
    void clearTable();

signals:
    // 👉 ha interaktívvá tesszük (pl. checkbox a teljesítéshez), ezen jelezhetünk vissza
    void relocationRowChecked(const QUuid& rowId, bool done);

public:
    static constexpr auto RowId_Key = "relocationId"; // 👉 UserRole kulcs a cellákban
};
