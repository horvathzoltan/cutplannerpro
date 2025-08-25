#pragma once

#include <QObject>
#include <QTableWidget>
#include "model/storageaudit/storageauditentry.h" // vagy ahová az entry kerül

class StorageAuditTableManager : public QObject {
    Q_OBJECT

public:
    explicit StorageAuditTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const StorageAuditEntry& entry);
    void clearTable();

private:
    QTableWidget* table;
    QWidget* parent;

    static constexpr int ColMaterial   = 0;
    static constexpr int ColStorage    = 1;
    static constexpr int ColExpected   = 2;
    static constexpr int ColPresent    = 3;
    static constexpr int ColActual     = 4;
    static constexpr int ColMissing    = 5;
};

