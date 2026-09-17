#pragma once

#include <QObject>

class MaterialStorageGroupRepository : public QObject
{
    Q_OBJECT

public:
    explicit MaterialStorageGroupRepository(QObject* parent = nullptr)
        : QObject(parent)
    {}

    // 🔥 Tárolási anyagcsoportok generálása gyártási csoportokból
    static QMap<QUuid, QUuid> buildStorageGroups();
};
