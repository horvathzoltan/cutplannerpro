#pragma once
#include "materials/model/material_family.h"
#include <QString>

class MaterialFamilyUtils {
public:
    static QString toString(MaterialFamily f) {
        switch (f) {
        case MaterialFamily::Tok:        return "Tok";
        case MaterialFamily::TokFed:     return "TokFed";
        case MaterialFamily::Zaro:       return "Zaro";
        case MaterialFamily::Lab:        return "Lab";
        case MaterialFamily::Palca:      return "Palca";
        case MaterialFamily::Tengely:    return "Tengely";
        case MaterialFamily::Suly:       return "Suly";
        case MaterialFamily::Motor:      return "Motor";
        case MaterialFamily::HajtoElem:  return "HajtoElem";
        case MaterialFamily::Rugo:       return "Rugo";
        case MaterialFamily::Fitting:    return "Fitting";
        case MaterialFamily::Kiegeszito: return "Kiegeszito";
        case MaterialFamily::Textil:     return "Textil";
        case MaterialFamily::TokVeg:     return "TokVeg";
        case MaterialFamily::Csavar:     return "Csavar";
        case MaterialFamily::Bowden:     return "Bowden";
        case MaterialFamily::FelsoSin:   return "FelsoSin";

        default:                         return "Unknown";
        }
    }

    static MaterialFamily fromString(const QString& s) {
        const QString v = s.trimmed().toLower();

        if (v == "tok")        return MaterialFamily::Tok;
        if (v == "tokfed")     return MaterialFamily::TokFed;
        if (v == "zaro")       return MaterialFamily::Zaro;
        if (v == "lab")        return MaterialFamily::Lab;
        if (v == "palca")      return MaterialFamily::Palca;
        if (v == "tengely")    return MaterialFamily::Tengely;
        if (v == "suly")       return MaterialFamily::Suly;
        if (v == "motor")      return MaterialFamily::Motor;
        if (v == "hajtoelem")  return MaterialFamily::HajtoElem;
        if (v == "rugo")       return MaterialFamily::Rugo;
        if (v == "fitting")    return MaterialFamily::Fitting;
        if (v == "kiegeszito") return MaterialFamily::Kiegeszito;
        if (v == "textil")     return MaterialFamily::Textil;
        if (v == "tokveg")     return MaterialFamily::TokVeg;
        if (v == "csavar")     return MaterialFamily::Csavar;
        if (v == "bowden")   return MaterialFamily::Bowden;
        if (v == "felsosin") return MaterialFamily::FelsoSin;

        return MaterialFamily::Unknown;
    }

    static inline bool matchPrefix(const QString& barcode, const QString& prefix)
    {
        QString bc = barcode.trimmed();
        QString px = prefix.trimmed();

        // ha '*' van a végén → prefix-család
        bool wildcard = px.endsWith("*");
        if (wildcard)
            px.chop(1);   // NP-CL* → NP-CL

        // exact match
        if (bc.compare(px, Qt::CaseInsensitive) == 0)
            return true;

        // wildcard esetén: csak "px-" kezdetű lehet
        if (wildcard) {
            if (bc.startsWith(px + "-", Qt::CaseInsensitive))
                return true;
            return false;
        }

        // normál prefix → NINCS wildcard → NINCS prefix-match
        // csak exact match engedélyezett
        return false;
    }
};
