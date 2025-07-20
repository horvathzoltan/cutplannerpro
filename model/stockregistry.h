#pragma once

#include <QVector>
#include <QUuid>
#include "stockentry.h"

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

    bool removeByMaterialId(const QUuid& id);

    QVector<StockEntry> findByGroupName(const QString& name) const;
};
