#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QUuid>

#include "materials/registry/material_registry.h"
#include "materials/model/material_master.h"

/// Egy bundle komponens leftover hossza.
/// -1 = teljes hossz (megegyezik a leftover availableLength_mm értékével)
///  0 = hiányzik
/// >0 = eltérő leftover hossz
struct BundleComponentLength {
    QUuid materialId;      // komponens anyag ID (GUID)
    int length_mm;         // komponens leftover hossza
};

namespace BundleComponentLengthUtils {

/// CSV formátum:
/// barcode:length,barcode:length,...
/// Példa:
/// NP-CL:-1,-1|NP-CLT:-1,-1|NP-CLB:0,0
inline QString toCsv(const QVector<BundleComponentLength>& list)
{
    if (list.isEmpty())
        return QString();

    // barcode → list of lengths
    QMap<QString, QStringList> grouped;

    for (const auto& c : list)
    {
        const MaterialMaster* m =
            MaterialRegistry::instance().findById(c.materialId);

        QString barcode = m ? m->barcode : QStringLiteral("UNKNOWN");

        grouped[barcode].append(QString::number(c.length_mm));
    }

    QStringList parts;
    for (auto it = grouped.begin(); it != grouped.end(); ++it)
    {
        QString barcode = it.key();
        QString lengths = it.value().join(",");   // len1;len2;len3
        parts << QString("%1:%2").arg(barcode).arg(lengths);
    }

    return parts.join("|");   // pontosan így beszéltük
}


/// CSV → modell
/// barcode:length → materialId:length
inline QVector<BundleComponentLength> fromCsv(const QString& csv)
{
    QVector<BundleComponentLength> out;

    if (csv.trimmed().isEmpty())
        return out;

    const auto items = csv.split('|', Qt::SkipEmptyParts);

    for (const auto& item : items)
    {
        const auto kv = item.split(':', Qt::SkipEmptyParts);
        if (kv.size() != 2)
            continue;

        QString barcode = kv[0].trimmed();
        QString lenStr  = kv[1].trimmed();

        const MaterialMaster* m =
            MaterialRegistry::instance().findByBarcode(barcode);

        if (!m)
            continue;

        const auto lenParts = lenStr.split(',', Qt::SkipEmptyParts);

        for (const auto& lp : lenParts)
        {
            int len = lp.toInt();
            out.append(BundleComponentLength{ m->id, len });
        }
    }

    return out;
}


} // namespace BundleComponentLengthUtils
