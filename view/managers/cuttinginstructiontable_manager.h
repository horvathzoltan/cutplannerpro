#pragma once

#include <QObject>
#include <QTableWidget>
#include <QMap>
#include <QUuid>

#include "model/cutting/instruction/cutinstruction.h"

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
    int _currentStepId = 1;   ///< aktuális sor stepId (sorvezetőhöz)
    int _currentRowIx = -1;   // aktuális sor indexe
public:
    explicit CuttingInstructionTableManager(QTableWidget* table,
                                            QWidget* parent = nullptr);

    /// Új machine beszúrása a táblába
    void addMachineRow(const MachineHeader& m);

    /// Új sor beszúrása a táblába
    void addRow(const CutInstruction& ci);

    /// Meglévő sor frissítése rowId alapján
    void updateRow(const QUuid& rowId, const CutInstruction& ci);

    /// Teljes tábla törlése
    void clearTable();

    int currentStepId() const {return _currentStepId;}
    int currentRowIx() const {return _currentRowIx;}

public slots:
    /// Sor végrehajtása (Finalize gomb)
    void finalizeRow(const QUuid& rowId);

signals:
    /// Jelzés: egy sor sikeresen finalizálva lett
    void rowFinalized(int rowIndex);
};

