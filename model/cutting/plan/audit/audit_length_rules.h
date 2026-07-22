#pragma once

#include <QVector>
#include <QStringList>
#include <QUuid>
#include "model/cutting/plan/request.h"

struct LengthAuditResult {
    bool hasError = false;
    QStringList messages;
};

class AuditLengthRules {
public:
    static LengthAuditResult check(
        const QVector<Cutting::Plan::Request>& list,
        const QUuid& typeId,
        const QUuid& subtypeId);
};
