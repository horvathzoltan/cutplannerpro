#pragma once
#include <QString>
#include "materials/model/material_family_detector.h"
#include "naphalo_prefix_match.h"   // ahol a matchPrefix van

static inline QString profilePostfixFor(const QString& barcode)
{
    // CIPZÁROS LÁB
    if (matchPrefix(barcode, "NP-CL"))
        return "27 cm"; // láb + takaró = 18+9

    // SÍNES LÁB
    if (matchPrefix(barcode, "NP-SL"))
        return "13 cm";

    // CIPZÁROS LÁBBETÉT
    // ezt nem festjük
    //if (matchPrefix(barcode, "NP-CLB") || matchPrefix(barcode, "NP-CLBR"))
    //    return "17 cm (betét)";

    // CIPZÁROS ZÁRÓ
    if (matchPrefix(barcode, "NP-CZ"))
        return "13 mm";

    // SÍNES ZÁRÓ
    if (matchPrefix(barcode, "NP-SZ"))
        return "11 mm";

    // POFA
    if (matchPrefix(barcode, "NP-POF"))
        return "10×10 cm";

    // TOKFEDÉL CSAVAR
    if (matchPrefix(barcode, "NP-CSAV"))
        return "Ø10 mm";

    // Egyéb anyagokhoz nincs postfix
    return "";
}
