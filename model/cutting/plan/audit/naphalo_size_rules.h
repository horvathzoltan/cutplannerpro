#pragma once

#include <QVector>
#include <QStringList>
#include "model/cutting/plan/request.h"

struct NaphaloSizeAuditResult {
    bool hasError = false;
    QStringList messages;
    int totalLength = 0;
};

class NaphaloSizeRules {
public:
    static NaphaloSizeAuditResult check(const QVector<Cutting::Plan::Request>& list);
};
