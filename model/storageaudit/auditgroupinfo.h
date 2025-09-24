#pragma once

#include <QString>
#include <QList>
#include <QUuid>

struct AuditGroupInfo {
private:
    QString _groupKey;           // 🔑 Csoport kulcs (pl. storageName vagy auditGroup)
    QList<QUuid> _rowIds;        // 📋 Audit sorok azonosítói ebben a csoportban

public:
    // --- Konstruktorok ---
    AuditGroupInfo() = default;
    explicit AuditGroupInfo(const QString& key) : _groupKey(key) {}

    // --- Getterek ---
    const QString& groupKey() const { return _groupKey; }
    const QList<QUuid>& rowIds() const { return _rowIds; }

    // --- Műveletek ---
    void addRow(const QUuid& rowId) {
        if (!_rowIds.contains(rowId))
            _rowIds.append(rowId);
    }

    bool isGroup() const {
        return _rowIds.size() > 1;
    }

    bool isEmpty() const {
        return _rowIds.isEmpty();
    }

    int size() const {
        return _rowIds.size();
    }
};
