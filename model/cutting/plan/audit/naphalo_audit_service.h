#pragma once

#include <QVector>
#include <QString>
#include <QHash>
#include "model/cutting/plan/request.h"
#include "naphalo_audit_types.h"

struct NaphaloAuditMessage {
    QString ref;
    QString text;
    bool isError = false;
};

struct NaphaloAuditResult {
    QVector<NaphaloAuditMessage> messages;
    QHash<QString, CountPerType> summary;
};

class NaphaloAuditService {
public:
    static QHash<QString, QVector<Cutting::Plan::Request>>
    groupByExternalRef(const QVector<Cutting::Plan::Request>& all);

    static NaphaloAuditResult run(const QVector<Cutting::Plan::Request>& all);
};

