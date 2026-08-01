#include "audit_length_rules.h"
#include "product/material_role_utils.h"
#include "materials/registry/material_registry.h"
//#include "materials/model/material_family_utils.h"

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

// NAPHÁLÓ típus felismerés (egyszerűsített)
static bool isNaphalo(const QUuid& typeId, const QUuid& subtypeId)
{
    auto* st0 = ProductTypeRegistry::instance().findById(typeId);
    if(!st0)
        return false;
    if(st0->code != "NP")
        return false;

    auto* st = ProductSubtypeRegistry::instance().findById(subtypeId);
    if (!st) return false;

    return (st->code == "CIP" || st->code == "SZ" || st->code == "BOW");
}

LengthAuditResult AuditLengthRules::check(
    const QVector<Cutting::Plan::Request>& list,
    const QUuid& typeId,
    const QUuid& subtypeId)
{
    LengthAuditResult r;

    if (list.isEmpty())
        return r;

    // csak naphálóra fut
    if (!isNaphalo(typeId, subtypeId))
        return r;

    const auto& first = list.first();

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
        bool lenValid = (len > 0);

        r.entries.add(
            req.externalReference.trimmed(),
            "LENGTH",
            QString("length > 0 for role %1").arg(key),
            lenValid,
            "> 0",
            QString("%1 (material %2)").arg(len).arg(mm->barcode)
            );

        if (lenValid)
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

    // tok <= tokfed
    bool tokTokfedValid = !(tok > 0 && tokfed > 0 && tok > tokfed);
    r.entries.add(
        first.externalReference.trimmed(),
        "LENGTH",
        "tok <= tokfed",
        tokTokfedValid,
        QString::number(tokfed),
        QString::number(tok)
        );

    // tengely < tok
    bool tengelyTokValid = !(tengely > 0 && tok > 0 && tengely >= tok);
    r.entries.add(
        first.externalReference.trimmed(),
        "LENGTH",
        "tengely < tok",
        tengelyTokValid,
        QString::number(tok),
        QString::number(tengely)
        );

    // zaro < tengely
    bool zaroTengelyValid = !(zaro > 0 && tengely > 0 && zaro >= tengely);
    r.entries.add(
        first.externalReference.trimmed(),
        "LENGTH",
        "zaro < tengely",
        zaroTengelyValid,
        QString::number(tengely),
        QString::number(zaro)
        );

    // suly < zaro
    bool sulyZaroValid = !(suly > 0 && zaro > 0 && suly >= zaro);
    r.entries.add(
        first.externalReference.trimmed(),
        "LENGTH",
        "suly < zaro",
        sulyZaroValid,
        QString::number(zaro),
        QString::number(suly)
        );

    // --- 4) Szélesség / magasság ellenőrzés ---
    int width  = first.fullWidth_mm;
    int height = first.fullHeight_mm;

    if (width > 0) {
        bool tokfedWidthValid = !(tokfed > 0 && tokfed > width);
        r.entries.add(
            first.externalReference.trimmed(),
            "LENGTH",
            "tokfed <= width",
            tokfedWidthValid,
            QString::number(width),
            QString::number(tokfed)
            );

        if (!tokfedWidthValid)
            r.hasValidDimensions = false;
    }

    if (height > 0) {
        bool labHeightValid = !(lab > 0 && lab > height);
        r.entries.add(
            first.externalReference.trimmed(),
            "LENGTH",
            "lab <= height",
            labHeightValid,
            QString::number(height),
            QString::number(lab)
            );

        if (!labHeightValid)
            r.hasValidDimensions = false;
    }

    // --- 5) Láb / lábtakaró / lábbetét részletes ellenőrzés ---

    // Csak akkor vizsgálunk, ha van legalább CL vagy CLT vagy CLB
    bool labRulesValid = true;

    // 1) CL és CLT: pontosan egyenlőnek kell lenniük (ha mindkettő szerepel)
    if (labCL > 0 && labCLT > 0) {
        bool cl_clt_equal = (labCL == labCLT);

        r.entries.add(
            first.externalReference.trimmed(),
            "LENGTH",
            "láb (CL) és lábtakaró (CLT) egyenlő hosszú",
            cl_clt_equal,
            QString("CL=%1, CLT=%2").arg(labCL).arg(labCLT),
            cl_clt_equal ? "OK" : "nem egyenlő"
            );

        if (!cl_clt_equal)
            labRulesValid = false;
    }

    // 2) CLB: nem lehet nagyobb, mint CL
    if (labCL > 0 && labCLB > 0) {
        bool clb_not_bigger = (labCLB <= labCL);

        r.entries.add(
            first.externalReference.trimmed(),
            "LENGTH",
            "lábbetét (CLB) nem lehet nagyobb mint a láb (CL)",
            clb_not_bigger,
            QString("CL=%1").arg(labCL),
            QString("CLB=%1").arg(labCLB)
            );

        if (!clb_not_bigger)
            labRulesValid = false;
    }

    // 3) CLB: nem lehet 2 mm-nél jobban kisebb
    if (labCL > 0 && labCLB > 0) {
        bool clb_not_too_small = (labCLB >= labCL - 2);

        r.entries.add(
            first.externalReference.trimmed(),
            "LENGTH",
            "lábbetét (CLB) max. 2 mm-rel lehet kisebb mint a láb (CL)",
            clb_not_too_small,
            QString("CL=%1").arg(labCL),
            QString("CLB=%1").arg(labCLB)
            );

        if (!clb_not_too_small)
            labRulesValid = false;
    }

    // Ha bármelyik láb-szabály sérül → méret érvénytelen
    if (!labRulesValid)
        r.hasValidDimensions = false;

    return r;
}

