#pragma once

#include <QList>
#include <QUuid>


struct AuditContext {
    QUuid materialId;
    QString groupKey; // pl. storageGroup vagy auditGroup
    int totalExpected = 0;
    int totalActual = 0;
    QList<QUuid> rowIds; // az audit sorok azonosítói
};
