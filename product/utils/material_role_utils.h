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