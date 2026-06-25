#pragma once

#include <QStringList>
#include <QVector>
#include "model/cutting/plan/request.h"

struct NaphaloOwnerAuditResult {
    bool hasError = false;
    QStringList messages;
    QString expectedOwner;
};

class NaphaloOwnerRules {
public:
    static NaphaloOwnerAuditResult check(const QVector<Cutting::Plan::Request>& list);
};
