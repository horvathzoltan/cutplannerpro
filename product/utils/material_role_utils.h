#pragma once

#include "materials/model/material_master.h"
#include "model/cutting/plan/request.h"
#include "product/model/material_role.h"


namespace MaterialRoleUtils{

inline QString normalizePrefix(const QString& barcode)
{
    QStringList parts = barcode.split('-');
    if (parts.size() < 2)
        return barcode;

    QString first = parts[0];
    QString second = parts[1];

    // 🔹 Bundle eset: NP-CL2+CLT2+CLB2* → NP-CL2+CLT2+CLB2
    // Ha van '+' a második részben, akkor a teljes második részt használjuk,
    // opcionálisan levágva a végéről a '*' wildcardot.
    if (second.contains('+')) {
        QString s2 = second;
        int starIx = s2.indexOf('*');
        if (starIx >= 0)
            s2 = s2.left(starIx);
        return first + "-" + s2;
    }

    // 🔹 Nem bundle: régi logika marad
    // tisztán betűs → teljes prefix
    bool allLetters = true;
    for (QChar c : second)
        if (!c.isLetter()) {
            allLetters = false; break;
        }

    // tisztán betűs → teljes prefix (pl. NP-T, NP-CZ)
    if (allLetters)
        return first + "-" + second;

    // vegyes → csak betűs rész (pl. NP-ROLL70 → NP-ROLL)
    QString letters;
    for (QChar c : second)
        if (c.isLetter()) letters.append(c);
        else break;

    return first + "-" + letters;
}

inline MaterialRole makeRole(const Cutting::Plan::Request& req,
                             const MaterialMaster* m)
{
    MaterialRole r;
    r.productTypeId = req.productTypeId;
    r.productSubtypeId = req.productSubtypeId;
    r.family = m->family;
    r.barcodePrefix = normalizePrefix(m->barcode);
    return r;
}

} //end namespace MaterialFamilyUtils