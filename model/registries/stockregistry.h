#pragma once

#include <QVector>
#include <QUuid>
#include "../stockentry.h"

class StockRegistry {
private:
    StockRegistry() = default;

    QVector<StockEntry> _stock;
public:
    // 🔁 Singleton elérés
    static StockRegistry& instance() {
        static StockRegistry reg;
        return reg;
    }

    void add(const StockEntry& entry);
    void clear();
    QVector<StockEntry> all() const { return _stock; }

    void remove(const QUuid& id);

    QVector<StockEntry> findByGroupName(const QString& name) const;

    void consume(const QUuid& materialId); // 🧱 Levon egy darabot a készletből az adott anyaghoz
    void persist() const;

    std::optional<StockEntry> findById(const QUuid& entryId) const; // ⬅️ új

    bool update(const StockEntry &updated);
};
