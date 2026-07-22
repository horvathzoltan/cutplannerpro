#include "audit_length_rules.h"
#include "product/material_role_utils.h"
#include "materials/registry/material_registry.h"
#include "materials/model/material_family_utils.h"

#include <product/registry/product_subtype_registry.h>

// NAPHÁLÓ típus felismerés (egyszerűsített)
static bool isNaphalo(const QUuid& typeId, const QUuid& subtypeId)
{
    // Ha később lesz registry, ide kerül.
    // Most: minden CIP / SZ / BOW altípus naphálós.
    auto* st = ProductSubtypeRegistry::instance().findById(subtypeId);
    if (!st) return false;

    QString code = st->code.trimmed();
    return (code == "CIP" || code == "SZ" || code == "BOW");
}

LengthAuditResult AuditLengthRules::check(
    const QVector<Cutting::Plan::Request>& list,
    const QUuid& typeId,
    const QUuid& subtypeId)
{
    LengthAuditResult r;

    if (list.isEmpty())
        return r;

    if (!isNaphalo(typeId, subtypeId))
        return r;   // csak naphálóra fut

    // --- 1) Role-alapú hossz aggregálás ---
    QHash<QString, int> maxLen;   // role → max hossz

    for (const auto& req : list)
    {
        const MaterialMaster* mm =
            MaterialRegistry::instance().findById(req.materialId);

        if (!mm)
            continue;

        MaterialRole role = MaterialRoleUtils::makeRole(req, mm);
        QString key = role.barcodePrefix.trimmed();

        int len = req.requiredLength;
        if (len <= 0) {
            r.messages << QString("❌ Érvénytelen hossz (%1 mm) anyag: %2")
                              .arg(len).arg(mm->barcode);
            r.hasError = true;
        }

        maxLen[key] = qMax(maxLen.value(key, 0), len);
    }

    // --- 2) Role-kulcsok ---
    int tok     = maxLen.value("NP-T",    0);
    int tokfed  = maxLen.value("NP-TF",   0);
    int tengely = maxLen.value("NP-ROLL", 0);

    int zaroCIP = maxLen.value("NP-CZ",   0);
    int zaroSZ  = maxLen.value("NP-SZ",   0);
    int zaro    = qMax(zaroCIP, zaroSZ);

    int suly    = maxLen.value("NP-BAR",  0);

    int labCL  = maxLen.value("NP-CL",  0);
    int labCLT = maxLen.value("NP-CLT", 0);
    int labCLB = maxLen.value("NP-CLB", 0);

    int lab = qMax(labCL, qMax(labCLT, labCLB));


    // --- 3) Méret-összefüggések ---
    if (tok > 0 && tokfed > 0 && tok > tokfed) {
        r.messages << QString("❌ Tok (%1 mm) nagyobb mint tokfedél (%2 mm)")
                          .arg(tok).arg(tokfed);
        r.hasError = true;
    }

    if (tengely > 0 && tok > 0 && tengely >= tok) {
        r.messages << QString("❌ Tengely (%1 mm) nem lehet nagyobb vagy egyenlő a toknál (%2 mm)")
                          .arg(tengely).arg(tok);
        r.hasError = true;
    }

    if (zaro > 0 && tengely > 0 && zaro >= tengely) {
        r.messages << QString("❌ Záró (%1 mm) nem lehet nagyobb vagy egyenlő a tengelynél (%2 mm)")
                          .arg(zaro).arg(tengely);
        r.hasError = true;
    }

    if (suly > 0 && zaro > 0 && suly >= zaro) {
        r.messages << QString("❌ Súly (%1 mm) nem lehet nagyobb vagy egyenlő a zárónál (%2 mm)")
                          .arg(suly).arg(zaro);
        r.hasError = true;
    }

    // --- 4) Szélesség / magasság ellenőrzés ---
    int width  = list.first().fullWidth_mm;
    int height = list.first().fullHeight_mm;

    if(width>0){
        if (tokfed > 0 && tokfed > width) {
            r.messages << QString("❌ Tokfedél (%1 mm) nagyobb mint a szélesség (%2 mm)")
                              .arg(tokfed).arg(width);
            r.hasError = true;
        }
    }

    if(height>0){
        if (lab > 0 && lab > height) {
            r.messages << QString("❌ Láb (%1 mm) nagyobb mint a magasság (%2 mm)")
                              .arg(lab).arg(height);
            r.hasError = true;
        }
    }

    return r;
}
