#pragma once

#include <QVector>
#include <QUuid>
#include "../stockentry.h"

class StockRegistry {
private:
    StockRegistry() = default;
    StockRegistry(const StockEntry&) = delete;

    QVector<StockEntry> _data;
    //bool isPersist = true; // 📁 Perzisztálás engedélyezése
    void persist() const;
public:
    // 🔁 Singleton elérés
    static StockRegistry& instance();

    void registerEntry(const StockEntry& entry);
    bool updateEntry(const StockEntry &updated);
    void consumeEntry(const QUuid& materialId); // 🧱 Levon egy darabot a készletből az adott anyaghoz
    void removeEntry(const QUuid& id);

    QVector<StockEntry> readAll() const { return _data; }
    void clearAll();

    std::optional<StockEntry> findById(const QUuid& entryId) const; // ⬅️ új
    QVector<StockEntry> findByGroupName(const QString& name) const;

    bool isEmpty() const { return _data.isEmpty(); }
    void setData(const QVector<StockEntry>& v) { _data = v;}
};
