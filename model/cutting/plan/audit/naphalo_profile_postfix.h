#pragma once
#include <QHash>
#include <QString>
#include <materials/model/material_family_utils.h>

namespace ProfileUtils
{
    static inline QString profilePostfixFor(const QString& role)
    {
        if(role.isEmpty())
            return "";

        // TOK
        if (MaterialFamilyUtils::matchPrefix(role, "NP-T"))
            return "20 cm";

        // TOKFEDÉL
        if (MaterialFamilyUtils::matchPrefix(role, "NP-TF"))
            return "18 cm";

        // láb + takaró = 18+9
        // CIPZÁROS LÁB
        if (MaterialFamilyUtils::matchPrefix(role, "NP-CL"))
            return "18 cm";

        if (MaterialFamilyUtils::matchPrefix(role, "NP-CLT"))
            return "9 cm";

        if (MaterialFamilyUtils::matchPrefix(role, "NP-CL2+CLT2+CLB2"))
            return "54 cm";

        if (MaterialFamilyUtils::matchPrefix(role, "NP-CL+CLT"))
            return "27 cm";

        // SÍNES LÁB
        if (MaterialFamilyUtils::matchPrefix(role, "NP-SL"))
            return "13 cm";

        // CIPZÁROS LÁBBETÉT
        // ezt nem festjük
        //if (matchPrefix(barcode, "NP-CLB") || matchPrefix(barcode, "NP-CLBR"))
        //    return "17 cm (betét)";

        // CIPZÁROS ZÁRÓ
        if (MaterialFamilyUtils::matchPrefix(role, "NP-CZ"))
            return "13 cm";

        // SÍNES ZÁRÓ
        if (MaterialFamilyUtils::matchPrefix(role, "NP-SZ"))
            return "11 cm";

        // POFA
        if (MaterialFamilyUtils::matchPrefix(role, "NP-POF"))
            return "10×10 cm";

        // TOKFEDÉL CSAVAR
        if (MaterialFamilyUtils::matchPrefix(role, "NP-CSAV"))
            return "Ø10 mm";

        // Egyéb anyagokhoz nincs postfix
        return "";
    }
}