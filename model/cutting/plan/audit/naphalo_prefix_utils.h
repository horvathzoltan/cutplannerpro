// #pragma once
// #include <QHash>
// #include <QString>
// #include "naphalo_material_aggregator.h"

// static inline int sumPrefix(const QHash<QString, MaterialAggregate>& map, const QString& prefix)
// {
//     int total = 0;
//     for (auto it = map.begin(); it != map.end(); ++it) {
//         const QString& key = it.key();
//         if (key == prefix || key.startsWith(prefix + "-"))
//             total += it.value().count;
//     }
//     return total;
// }
