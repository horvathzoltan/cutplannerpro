#include "naphalo_size_rules.h"
#include "naphalo_material_aggregator.h"

NaphaloSizeAuditResult NaphaloSizeRules::check(const QVector<Cutting::Plan::Request>& list)
{
    NaphaloSizeAuditResult r;

    if (list.isEmpty())
        return r;

    // 🔹 Egyszeri aggregálás
    auto materials = NaphaloMaterialAggregator::aggregate(list);

    // 🔹 Hosszellenőrzés a nyers listán (mert ez request-szintű)
    for (const auto& req : list) {
        int len = req.requiredLength;

        if (len <= 0) {
            r.messages << QString("⚠️ HIBA: Érvénytelen hossz (%1 mm)").arg(len);
            r.hasError = true;
        }

        if (len > 6000) { // például 6 méter a max
            r.messages << QString("⚠️ TÚL HOSSZÚ: %1 mm").arg(len);
            r.hasError = true;
        }
    }

    // 🔹 Összhossz számítása aggregátorból
    int total = 0;
    for (auto it = materials.begin(); it != materials.end(); ++it) {
        const MaterialAggregate& agg = it.value();
        total += agg.totalLength;
    }

    r.totalLength = total;

    // 🔹 Összhossz ellenőrzés
    if (total <= 0) {
        r.messages << "⚠️ HIBA: Összhossz = 0 mm";
        r.hasError = true;
    }

    return r;
}
