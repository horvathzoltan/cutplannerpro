#pragma once

#include <QObject>
#include <QTableWidget>
#include <QMap>
#include <QUuid>

#include "model/cutting/instruction/cutinstruction.h"
#include "view/viewmodels/cutting/rowgenerator.h"
#include "view/tablehelpers/tablerowpopulator.h"
#include "common/logger.h"
#include "model/registries/materialregistry.h"
#include "common/tableutils/colorlogicutils.h"

/**
 * @brief CuttingInstructionTableManager
 *
 * Feladata:
 *  - A QTableWidget sorainak kezelése (addRow, updateRow, clearTable)
 *  - A CutInstruction objektumokhoz tartozó ViewModel generálása
 *  - finalizeRow hívás kezelése (vágás végrehajtása, leftover regisztráció)
 */
class CuttingInstructionTableManager : public QObject {
    Q_OBJECT

private:
    QTableWidget* _table = nullptr;              ///< A tényleges QTableWidget példány
    QWidget* _parent = nullptr;                  ///< Parent widget (ownership chain)
    QMap<QUuid, CutInstruction> _rowMap;         ///< rowId → CutInstruction
    QMap<QUuid, int> _rowIndexMap;               ///< rowId → rowIndex gyors lookup
    static bool _isVerbose;                      ///< Debug log flag

public:
    explicit CuttingInstructionTableManager(QTableWidget* table,
                                            QWidget* parent = nullptr);

    /// Új sor beszúrása a táblába
    void addRow(const CutInstruction& ci);

    /// Meglévő sor frissítése rowId alapján
    void updateRow(const QUuid& rowId, const CutInstruction& ci);

    /// Teljes tábla törlése
    void clearTable();

public slots:
    /// Sor végrehajtása (Finalize gomb)
    void finalizeRow(const QUuid& rowId);
};
