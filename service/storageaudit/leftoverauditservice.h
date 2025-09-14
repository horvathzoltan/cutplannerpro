#pragma once

#include <QObject>
#include <QVector>
#include "model/storageaudit/storageauditrow.h"
#include "model/leftoverstockentry.h"

class LeftoverAuditService : public QObject {
    Q_OBJECT

public:
    static LeftoverAuditService& instance();

    //QVector<StorageAuditRow> generateAudit(); // teljes készlet audit
    //QVector<StorageAuditRow> generateAudit(const QVector<LeftoverStockEntry>& entries); // célzott audit

    QVector<StorageAuditRow> generateAudit(const QVector<LeftoverStockEntry> &entries, const QSet<QUuid> &usedInOptimization);
private:
    explicit LeftoverAuditService(QObject* parent = nullptr);
};
