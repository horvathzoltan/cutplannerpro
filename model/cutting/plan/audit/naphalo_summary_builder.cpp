#include "naphalo_summary_builder.h"

void NaphaloSummaryBuilder::update(
    QHash<QString, CountPerType>& summary,
    const QString& ref,
    const NaphaloOwnerAuditResult& owner,
    const NaphaloDueAuditResult& due,
    const NaphaloBomAuditResult& bom,
    const NaphaloSizeAuditResult& size,
    NaphaloType type
    ) {
    QString customer = owner.expectedOwner.isEmpty()
    ? "<ismeretlen>"
    : owner.expectedOwner;

    auto& s = summary[customer];

    s.total++;

    if (type == NaphaloType::Cipzaros) s.cipzaros++;
    if (type == NaphaloType::Sines)    s.sines++;
    if (type == NaphaloType::Bowdenes) s.bowdenes++;

    bool hasError =
        owner.hasError ||
        due.hasError   ||
        bom.hasError   ||
        size.hasError;

    if (hasError) {
        s.bad++;
        s.badRefs << ref;
    } else {
        s.good++;
    }
}
