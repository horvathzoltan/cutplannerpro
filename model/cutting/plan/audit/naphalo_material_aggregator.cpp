#include "naphalo_material_aggregator.h"
#include "materials/registry/material_registry.h"

QHash<QString, MaterialAggregate>
NaphaloMaterialAggregator::aggregate(const QVector<Cutting::Plan::Request>& list)
{
    QHash<QString, MaterialAggregate> map;

    for (const auto& req : list) {

        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(req.materialId);

        QString code = mat ? mat->barcode : "???";

        auto& agg = map[code];
        agg.count       += req.quantity;
        agg.totalLength += req.quantity * req.requiredLength;
    }

    return map;
}

int NaphaloMaterialAggregator::sumPrefix(const QHash<QString, MaterialAggregate>& map,
                                         const QString& prefix)
{
    int total = 0;

    for (auto it = map.begin(); it != map.end(); ++it) {
        const QString& key = it.key();

        // NP-CL vagy NP-CL-9010 vagy NP-CL-7016 stb.
        if (key == prefix || key.startsWith(prefix + "-"))
            total += it.value().count;
    }

    return total;
}

int NaphaloMaterialAggregator::sumGroup(
    const QHash<QString, MaterialAggregate>& map,
    NaphaloGroup group)
{
    int total = 0;

    for (const auto& prefix : GROUP_PREFIXES[group]) {
        total += sumPrefix(map, prefix);
    }

    return total;
}


int NaphaloMaterialAggregator::sumFamily(
    const QHash<QString, MaterialAggregate>& map,
    NaphaloFamily family)
{
    int total = 0;

    for (const auto& group : FAMILY_GROUPS[family]) {
        total += sumGroup(map, group);
    }

    return total;
}
