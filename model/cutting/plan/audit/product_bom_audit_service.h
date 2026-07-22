#pragma once

#include <QVector>
#include <QString>
#include <QHash>
#include "model/cutting/plan/request.h"
#include "naphalo_audit_types.h"

struct ProductBomAuditMessage {
    QString ref;
    QString text;
    bool isError = false;
};

struct ProductBomAuditSummary {
    QHash<QString, CountPerType> perCustomer;
};

struct ProductBomAuditResult {
    QVector<ProductBomAuditMessage> messages;
    ProductBomAuditSummary summary;
};

class ProductBomAuditService {
public:
    static ProductBomAuditResult run(const QVector<Cutting::Plan::Request>& all);
};


