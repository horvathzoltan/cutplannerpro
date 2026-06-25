#pragma once

#include <QString>
#include <QHash>
#include "naphalo_audit_types.h"
#include "naphalo_owner_rules.h"
#include "naphalo_due_rules.h"
#include "naphalo_bom_rules.h"
#include "naphalo_size_rules.h"
#include "naphalo_type_detector.h"

class NaphaloSummaryBuilder {
public:
    static void update(
        QHash<QString, CountPerType>& summary,
        const QString& ref,
        const NaphaloOwnerAuditResult& owner,
        const NaphaloDueAuditResult& due,
        const NaphaloBomAuditResult& bom,
        const NaphaloSizeAuditResult& size,
        NaphaloType type
        );
};
