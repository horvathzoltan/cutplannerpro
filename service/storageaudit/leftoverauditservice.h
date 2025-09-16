#pragma once

#include <QObject>
#include <QVector>
#include "model/storageaudit/storageauditrow.h"
//#include "model/leftoverstockentry.h"

class LeftoverAuditService : public QObject {
    Q_OBJECT

public:
    explicit LeftoverAuditService(QObject* parent = nullptr);
    static QVector<StorageAuditRow> generateAuditRows_All();

};
