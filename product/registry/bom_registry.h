#pragma once
#include <QVector>
#include <QUuid>
#include "product/model/bom_entry.h"

class BomRegistry {
private:
    BomRegistry() = default;
    QVector<BomEntry> _data;

public:
    static BomRegistry& instance();

    void setData(const QVector<BomEntry>& v) { _data = v; }
    const QVector<BomEntry>& readAll() const { return _data; }

    QVector<BomEntry> findByType(const QUuid& typeId) const {
        QVector<BomEntry> out;
        for (const auto& e : _data)
            if (e.productTypeId == typeId)
                out.append(e);
        return out;
    }

    QVector<BomEntry> findByTypeAndSubtype(const QUuid& typeId, const QUuid& subtypeId) const {
        QVector<BomEntry> out;
        for (const auto& e : _data)
            if (e.productTypeId == typeId && e.productSubtypeId == subtypeId)
                out.append(e);
        return out;
    }
};
