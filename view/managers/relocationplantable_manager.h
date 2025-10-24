#pragma once

#include <QObject>
#include <QTableWidget>
#include <QMap>
#include <QUuid>
#include "model/relocation/relocationinstruction.h"
#include "presenter/CuttingPresenter.h"

class RelocationPlanTableManager : public QObject {
    Q_OBJECT

private:
    QTableWidget* _table;   // 👉 the actual QTableWidget instance
    QWidget* _parent;       // 👉 parent widget, ha kell ownership chain

    // 👉 belső storage: rowId → RelocationInstruction
    QMap<QUuid, RelocationInstruction> _planRowMap;
    // 👉 gyors lookup: rowId → rowIndex (így nem kell végigiterálni a táblát update-nél)
    QMap<QUuid, int> _rowIndexMap;
    CuttingPresenter* _presenter = nullptr;   // 🔹 új mező

    static bool _isVerbose; // 👉 debug logging flag

    void overwriteRow(const QUuid &rowId, const RelocationInstruction &instr);
public:
    explicit RelocationPlanTableManager(QTableWidget* table,
                                        CuttingPresenter* presenter,
                                        QWidget* parent);

    // 👉 új sor beszúrása
    void addRow(const RelocationInstruction& instr);
    // 👉 meglévő sor frissítése rowId alapján
    void updateRow(const QUuid& rowId, const RelocationInstruction& instr);
    // 👉 teljes tábla törlése
    void clearTable();

public slots:
    void editRow(const QUuid& rowId, const QString& mode);
    void finalizeRow(const QUuid &rowId);

// signals:
//     // 👉 ha interaktívvá tesszük (pl. checkbox a teljesítéshez), ezen jelezhetünk vissza
//     void relocationRowChecked(const QUuid& rowId, bool done);

public:
    static constexpr auto RowId_Key = "relocationId"; // 👉 UserRole kulcs a cellákban
    void updateSummaryRow(const RelocationInstruction &summaryInstr);
    QVector<QUuid> allRowIds() const {
        return _planRowMap.keys();
    }

    const RelocationInstruction& getInstruction(const QUuid& rowId) const;
    RelocationInstruction& getInstruction(const QUuid& rowId);

}; // end of class RelocationPlanTableManager

