#pragma once
#include <QString>
#include <QUuid>
#include <QVector>

struct MaterialStorageGroup
{
    QUuid id;                 // egyedi ID
    QString name;             // emberi név (pl. "Tárolási NP-T")
    QString barcode;          // csoport kulcs (pl. "SG-NP-T")
    QVector<QUuid> members;   // anyagok GUID-jai

    MaterialStorageGroup() = default;

    MaterialStorageGroup(const QUuid& gid,
                         const QString& n,
                         const QString& bc,
                         const QVector<QUuid>& m)
        : id(gid), name(n), barcode(bc), members(m)
    {}
};
