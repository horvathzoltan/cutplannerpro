#pragma once

#include <QVector>
#include <QStringList>
#include "model/cutting/plan/request.h"

struct NaphaloBomAuditResult {
    bool hasError = false;
    QStringList messages;
    bool isCipzaros = false;
    bool isSines = false;
    bool isBowdenes = false;
    int tokCount = 0;
    int tokFedCount = 0;
    int labCount = 0;
    int labbetetCount = 0;
    int czCount = 0;
    int szCount = 0;
};

class NaphaloBomRules {
public:
    static NaphaloBomAuditResult check(const QVector<Cutting::Plan::Request>& list);
};
