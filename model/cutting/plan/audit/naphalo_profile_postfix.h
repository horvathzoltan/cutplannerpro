#pragma once
#include <QHash>
#include <QString>
#include <materials/model/material_family_utils.h>

namespace ProfileUtils
{
    static inline QString profilePostfixFor(const QString& barcode)
    {
        // TOK
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-T"))
            return "20 cm";

        // TOKFEDÉL
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-TF"))
            return "18 cm";

        // CIPZÁROS LÁB
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-CL"))
            return "27 cm"; // láb + takaró = 18+9

        // SÍNES LÁB
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-SL"))
            return "13 cm";

        // CIPZÁROS LÁBBETÉT
        // ezt nem festjük
        //if (matchPrefix(barcode, "NP-CLB") || matchPrefix(barcode, "NP-CLBR"))
        //    return "17 cm (betét)";

        // CIPZÁROS ZÁRÓ
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-CZ"))
            return "13 cm";

        // SÍNES ZÁRÓ
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-SZ"))
            return "11 cm";

        // POFA
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-POF"))
            return "10×10 cm";

        // TOKFEDÉL CSAVAR
        if (MaterialFamilyUtils::matchPrefix(barcode, "NP-CSAV"))
            return "Ø10 mm";

        // Egyéb anyagokhoz nincs postfix
        return "";
    }
}