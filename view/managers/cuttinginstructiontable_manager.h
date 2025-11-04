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

public slots:
    /// Sor végrehajtása (Finalize gomb)
    void finalizeRow(const QUuid& rowId);
};

// class HighlightDelegate : public QStyledItemDelegate {
// public:
//     int currentRow = -1;
//     void paint(QPainter* p, const QStyleOptionViewItem& opt,
//                const QModelIndex& idx) const override {
//         QStyledItemDelegate::paint(p, opt, idx);
//         if (idx.row() == currentRow) {
//             p->save();
//             QPen pen(Qt::red, 2);
//             p->setPen(pen);
//             p->drawRect(opt.rect.adjusted(0,0,-1,-1));
//             p->restore();
//         }
//     }
// };
