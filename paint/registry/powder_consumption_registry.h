#pragma once

#include <QHash>
#include <QString>
#include <QUuid>

#include "common/logger.h"
#include "paint/model/powder_consumption_model.h"

class PowderConsumptionRegistry
{
public:
    static PowderConsumptionRegistry& instance()
    {
        static PowderConsumptionRegistry inst;
        return inst;
    }

    void setData(const QVector<PowderConsumptionModel>& v)
    {
        _data = v;
        _map.clear();

        for (const auto& m : _data) {
            QString key = makeKey(m.productTypeId, m.productSubtypeId);
            _map[key] = m;
        }
    }

    bool isEmpty() const { return _data.isEmpty(); }

    PowderConsumptionModel find(const QUuid& typeId,
                                const QUuid& subtypeId) const
    {
        QString key = makeKey(typeId, subtypeId);
        if (!_map.contains(key))
            return PowderConsumptionModel{};
        return _map.value(key);
    }

private:
    PowderConsumptionRegistry() = default;

    QVector<PowderConsumptionModel> _data;
    QHash<QString, PowderConsumptionModel> _map;

    inline QString makeKey(const QUuid& t, const QUuid& s) const
    {
        return t.toString() + "-" + s.toString();
    }
};
