#include "naphalo_audit_service.h"

#include "naphalo_bom_rules.h"
#include "naphalo_size_rules.h"
#include "naphalo_type_detector.h"
#include "naphalo_material_aggregator.h"
#include "naphalo_summary_builder.h"
#include "naphalo_owner_rules.h"
#include "naphalo_due_rules.h"


#include <model/cutting/plan/request.h>

QHash<QString, QVector<Cutting::Plan::Request>>
NaphaloAuditService::groupByExternalRef(const QVector<Cutting::Plan::Request>& all)
{
    QHash<QString, QVector<Cutting::Plan::Request>> groups;

    for (const auto& req : all) {
        QString ref = req.externalReference.trimmed();
        if (ref.isEmpty())
            ref = "<NINCS_TETELSZAM>";
        groups[ref].append(req);
    }

    return groups;
}

NaphaloAuditResult NaphaloAuditService::run(const QVector<Cutting::Plan::Request>& all)
{
    NaphaloAuditResult result;

    auto groups = groupByExternalRef(all);

    for (auto it = groups.begin(); it != groups.end(); ++it) {

        const QString ref = it.key();
        const auto& list = it.value();

        auto materials = NaphaloMaterialAggregator::aggregate(list);

        // --- OWNER AUDIT ---
        auto owner = NaphaloOwnerRules::check(list);
        for (const QString& msg : owner.messages)
            result.messages.append({ ref, msg, owner.hasError });

        // --- DUE DATE AUDIT ---
        auto due = NaphaloDueRules::check(list);
        for (const QString& msg : due.messages)
            result.messages.append({ ref, msg, due.hasError });

        // --- BOM AUDIT ---
        auto bom = NaphaloBomRules::check(list);
        for (const QString& msg : bom.messages)
            result.messages.append({ ref, msg, bom.hasError });

        // --- SIZE AUDIT ---
        auto size = NaphaloSizeRules::check(list);
        for (const QString& msg : size.messages)
            result.messages.append({ ref, msg, size.hasError });

        // --- TYPE DETECTION ---
        auto type = NaphaloTypeDetector::detect(list);

        // --- SUMMARY UPDATE ---
        QString customer = owner.expectedOwner.isEmpty()
                               ? "<ismeretlen>"
                               : owner.expectedOwner;

        NaphaloSummaryBuilder::update(
            result.summary,
            ref,
            owner,
            due,
            bom,
            size,
            type
            );

    }

    return result;
}

