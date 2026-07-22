#pragma once

#include <QVector>
#include <QStringList>
#include "model/cutting/plan/request.h"

struct HeaderAuditResult {
    bool hasError = false;
    QStringList messages;
    bool hasValidDimensions = true;   // fontos!
};

class AuditHeaderRules {
public:
    static HeaderAuditResult check(const QVector<Cutting::Plan::Request>& list);
};
