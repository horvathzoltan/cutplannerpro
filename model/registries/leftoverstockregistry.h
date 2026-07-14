#pragma once

#include <QVector>
#include <QUuid>
#include "../leftoverstockentry.h"

class LeftoverStockRegistry {
private:
    LeftoverStockRegistry() = default;
    LeftoverStockRegistry(const LeftoverStockEntry&) = delete;

    QVector<LeftoverStockEntry> _data;
    //bool isPersist = true;

    void persist() const;
public:
    // 🔁 Singleton elérés
    static LeftoverStockRegistry& instance();

    void registerEntry(const LeftoverStockEntry& entry);
    bool updateEntry(const LeftoverStockEntry &updatedEntry);
    void consumeEntry(const QString& barcode);  // ♻️ Egy reusable darabot levon (töröl) a registry-ből
    bool removeEntry(const QUuid &entryId);
    //bool removeByMaterialId(const QUuid& id);
    void clearAll();

    QVector<LeftoverStockEntry> readAll() const { return _data; }
    std::optional<LeftoverStockEntry> findById(const QUuid &entryId) const;
    //QVector<LeftoverStockEntry> findByGroupName(const QString& name) const;

    QVector<LeftoverStockEntry> filtered(int minLength_mm) const;

    bool isEmpty() const { return _data.isEmpty(); }
    void setData(const QVector<LeftoverStockEntry>& v) { _data = v;}

    std::optional<LeftoverStockEntry> findByBarcode(const QString &barcode) const;
    bool existsBarcode(const QString& barcode, const QUuid& ignoreId = QUuid()) const;

    bool markSeen(const QUuid& entryId);
    bool markNotFound(const QUuid& entryId);

};
