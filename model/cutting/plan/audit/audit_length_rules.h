#pragma once

#include <QVector>
#include <QStringList>
#include <QUuid>
#include "model/cutting/plan/request.h"
#include "product_bom_audit_service.h"

struct LengthAuditResult {
    BomAuditEntries entries;
    bool hasValidDimensions = true;
};

class AuditLengthRules {
public:
    static LengthAuditResult check(
        const QVector<Cutting::Plan::Request>& list,
        const QUuid& typeId,
        const QUuid& subtypeId);
};
