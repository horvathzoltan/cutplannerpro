#pragma once
#include <QHash>
#include <QString>
#include <materials/model/material_family_utils.h>

namespace ProfileUtils
{
static inline QString profilePostfixFor_Role(const QString& groupKey)
{
    if (groupKey.isEmpty())
        return "";

    if (groupKey == "RNP-T")   return "20 cm";   // Tok
    if (groupKey == "RNP-TF")  return "18 cm";   // Tokfedél
    if (groupKey == "RNP-CZ")  return "13 cm";   // Cipzáros záró
    if (groupKey == "RNP-SZ")  return "11 cm";   // Sines záró

    if (groupKey == "RNP-SL2") return "26 cm";   // Sines láb
    if (groupKey == "RNP-SL") return "13 cm";   // Sines láb

    // Cipzáros láb (összetett)
    if (groupKey == "RNP-CL2+CLT2+CLB2") return "54 cm";
    if (groupKey == "RNP-CL") return "18 cm";
    if (groupKey == "RNP-CLT") return "9 cm";
    // Pofa
    if (groupKey == "RNP-POF") return "10×10 cm";

    // Csavar
    if (groupKey == "RNP-CSAV") return "Ø10 mm";

    QString e = "";
    return e;
}


// static inline QString profilePostfixFor_Material(const QString& groupKey)
// {
//     if (groupKey.isEmpty())
//         return "";

//     if (groupKey == "NP-T")   return "20 cm";   // Tok
//     if (groupKey == "NP-TF")  return "18 cm";   // Tokfedél
//     if (groupKey == "NP-CZ")  return "13 cm";   // Cipzáros záró
//     if (groupKey == "NP-SZ")  return "11 cm";   // Sines záró

//     if (groupKey == "NP-SL2") return "26 cm";   // Sines láb
//     if (groupKey == "NP-SL") return "13 cm";   // Sines láb

//     // Cipzáros láb (összetett)
//     if (groupKey == "NP-CL2+CLT2+CLB2") return "54 cm";
//     if (groupKey == "NP-CL") return "18 cm";
//     if (groupKey == "NP-CLT") return "9 cm";
//     // Pofa
//     if (groupKey == "NP-POF") return "10×10 cm";

//     // Csavar
//     if (groupKey == "NP-CSAV") return "Ø10 mm";

//     QString e = "";
//     return e;
// }

    // static inline QString profilePostfixFor(const QString& role)
    // {
    //     if(role.isEmpty())
    //         return "";

    //     // TOK
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-T"))
    //         return "20 cm";

    //     // TOKFEDÉL
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-TF"))
    //         return "18 cm";

    //     // láb + takaró = 18+9
    //     // CIPZÁROS LÁB
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-CL"))
    //         return "18 cm";

    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-CLT"))
    //         return "9 cm";

    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-CL2+CLT2+CLB2"))
    //         return "54 cm";

    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-CL+CLT"))
    //         return "27 cm";

    //     // SÍNES LÁB
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-SL"))
    //         return "13 cm";

    //     // CIPZÁROS LÁBBETÉT
    //     // ezt nem festjük
    //     //if (matchPrefix(barcode, "NP-CLB") || matchPrefix(barcode, "NP-CLBR"))
    //     //    return "17 cm (betét)";

    //     // CIPZÁROS ZÁRÓ
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-CZ"))
    //         return "13 cm";

    //     // SÍNES ZÁRÓ
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-SZ"))
    //         return "11 cm";

    //     // POFA
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-POF"))
    //         return "10×10 cm";

    //     // TOKFEDÉL CSAVAR
    //     if (MaterialFamilyUtils::matchPrefix(role, "NP-CSAV"))
    //         return "Ø10 mm";

    //     // Egyéb anyagokhoz nincs postfix
    //     return "";
    // }
}