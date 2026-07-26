#pragma once

#include <QVector>
#include <QStringList>
#include "model/cutting/plan/request.h"
#include "product_bom_audit_service.h"

struct HeaderAuditResult {
    bool hasValidDimensions = true;   // ez tényleg kell

    BomAuditEntries entries;   // 🔥 ÚJ
};

class AuditHeaderRules {
public:
    static HeaderAuditResult check(const QVector<Cutting::Plan::Request>& list);
};
