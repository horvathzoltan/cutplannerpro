#pragma once

#include <QStringList>
#include <QVector>
#include <QDate>
#include "model/cutting/plan/request.h"

struct NaphaloDueAuditResult {
    bool hasError = false;
    QStringList messages;
    QDate expectedDue;
};

class NaphaloDueRules {
public:
    static NaphaloDueAuditResult check(const QVector<Cutting::Plan::Request>& list);
};
