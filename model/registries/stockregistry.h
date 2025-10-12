#pragma once

#include <QVector>
#include <QUuid>
#include <QString>
#include <optional>
#include <QRecursiveMutex>

#include "model/stockentry.h"

class StockRegistry {
public:
    static StockRegistry& instance();

    // mutating operations (instant persist)
    void registerEntry(const StockEntry& entry);
    bool updateEntry(const StockEntry& updated);
    void consumeEntry(const QUuid& materialId);
    void removeEntry(const QUuid& id);
    void clearAll();

    // queries
    std::optional<StockEntry> findById(const QUuid& entryId) const;
    std::optional<StockEntry> findByMaterialAndStorage(const QUuid& materialId, const QUuid& storageId) const;
    QVector<StockEntry> findByMaterialId(const QUuid& materialId) const;
    QVector<StockEntry> findByGroupName(const QString& name) const;
    QVector<StockEntry> findAllByMaterialAndStorageSorted(const QUuid& materialId, const QUuid& storageId) const;
    std::optional<StockEntry> findFirstByStorageAndMaterial(const QUuid& storageId, const QUuid& materialId) const;

    // debug / persistence
    void dumpAll() const;
    void persist() const; // instant persist - feltételezzük, hogy caller lockolta a registry-t (debug warning lehetséges)

    // convenience
    bool isEmpty() const;
    QVector<StockEntry> readAll() const;
    void setData(const QVector<StockEntry>& v, bool doPersist = true);

private:
    StockRegistry() = default;
    ~StockRegistry() = default;
    StockRegistry(const StockRegistry&) = delete;
    StockRegistry& operator=(const StockRegistry&) = delete;

private:
    QVector<StockEntry> _data;
    mutable QRecursiveMutex _mutex; // reentrancia engedélyezése ugyanazon szálnak
};
