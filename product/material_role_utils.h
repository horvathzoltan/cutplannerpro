#pragma once

#include "materials/model/material_master.h"
#include "model/cutting/plan/request.h"
#include "product/model/material_role.h"


namespace MaterialRoleUtils{

static QString normalizePrefix(const QString& barcode)
{
    QStringList parts = barcode.split('-');
    if (parts.size() < 2)
        return barcode;

    QString first = parts[0];
    QString second = parts[1];

    // tisztán betűs → teljes prefix
    bool allLetters = true;
    for (QChar c : second)
        if (!c.isLetter()) { allLetters = false; break; }

    if (allLetters)
        return first + "-" + second;

    // vegyes → csak betűs rész
    QString letters;
    for (QChar c : second)
        if (c.isLetter()) letters.append(c);
        else break;

    return first + "-" + letters;
}

static MaterialRole makeRole(const Cutting::Plan::Request& req,
                             const MaterialMaster* m)
{
    MaterialRole r;
    r.productTypeId = req.productTypeId;
    r.productSubtypeId = req.productSubtypeId;
    r.family = m->family;

    // QString bc = m->barcode;
    // QStringList parts = bc.split('-');

    // if (parts.size() < 2) {
    //     r.barcodePrefix = bc;
    //     return r;
    // }

    // QString first = parts[0];      // NP
    // QString second = parts[1];     // CL, CLB, CLT, ROLL70, ROLL78, BAR22, ...

    // // 1) Ha a második tag tisztán betűs → ez a szerep (CL, CLB, CLT, CZ, T, TF)
    // bool allLetters = true;
    // for (QChar c : second) {
    //     if (!c.isLetter()) {
    //         allLetters = false;
    //         break;
    //     }
    // }

    // if (allLetters) {
    //     r.barcodePrefix = first + "-" + second;   // NP-CL, NP-CLB, NP-CLT, NP-CZ, NP-T, NP-TF
    //     return r;
    // }

    // // 2) Ha a második tag NEM tisztán betűs → a betűs rész a szerep (ROLL70 → ROLL)
    // QString letters;
    // for (QChar c : second) {
    //     if (c.isLetter())
    //         letters.append(c);
    //     else
    //         break;
    // }

    // r.barcodePrefix = first + "-" + letters;      // NP-ROLL, NP-BAR
    r.barcodePrefix = normalizePrefix(m->barcode);

    return r;
}





} //end namespace MaterialFamilyUtils