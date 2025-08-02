#pragma once

#include <QVector>
#include <QUuid>
#include "../leftoverstockentry.h"

class LeftoverStockRegistry {
private:
    LeftoverStockRegistry() = default;;

    QVector<LeftoverStockEntry> _stock;

public:
    // 🔁 Singleton elérés
    static LeftoverStockRegistry& instance() {
        static LeftoverStockRegistry reg;
        return reg;
    }

    void add(const LeftoverStockEntry& entry);
        void clear();
    QVector<LeftoverStockEntry> all() const { return _stock; }

    bool removeByMaterialId(const QUuid& id);

    QVector<LeftoverStockEntry> findByGroupName(const QString& name) const;

    void consume(const QString& barcode);  // ♻️ Egy reusable darabot levon (töröl) a registry-ből

    QVector<LeftoverStockEntry> filtered(int minLength_mm) const;

    void persist() const;
    bool removeByEntryId(const QUuid &entryId);
    bool update(const LeftoverStockEntry &updatedEntry);
    std::optional<LeftoverStockEntry> findById(const QUuid &entryId) const;
};
