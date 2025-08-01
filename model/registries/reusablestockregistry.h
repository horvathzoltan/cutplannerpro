#pragma once

#include <QVector>
#include <QUuid>
#include "../reusablestockentry.h"

class ReusableStockRegistry {
private:
    ReusableStockRegistry() = default;;

    QVector<ReusableStockEntry> _stock;

public:
    // 🔁 Singleton elérés
    static ReusableStockRegistry& instance() {
        static ReusableStockRegistry reg;
        return reg;
    }

    void add(const ReusableStockEntry& entry);
        void clear();
    QVector<ReusableStockEntry> all() const { return _stock; }

    bool removeByMaterialId(const QUuid& id);

    QVector<ReusableStockEntry> findByGroupName(const QString& name) const;

    void consume(const QString& barcode);  // ♻️ Egy reusable darabot levon (töröl) a registry-ből

    QVector<ReusableStockEntry> filtered(int minLength_mm) const;

    void persist() const;
};
