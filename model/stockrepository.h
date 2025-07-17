#pragma once

#include <QVector>
#include <QUuid>
#include "stockentry.h"

class StockRepository {
public:
    static StockRepository& instance();

    void add(const StockEntry& entry);
    void clear();
    QVector<StockEntry> all() const;
    bool removeByMaterialId(const QUuid& id);

    QVector<StockEntry> findByCategory(ProfileCategory cat) const;

private:
    QVector<StockEntry> _stock;

    StockRepository() = default;
};
