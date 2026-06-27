#pragma once
#include <QString>
#include "materials/model/material_family.h"

// ------------------------------------------------------------
//  Prefix matcher (case-insensitive, exact or prefix + '-')
// ------------------------------------------------------------
static inline bool matchPrefix(const QString& barcode, const QString& prefix)
{
    const QString bc = barcode.trimmed();
    const QString px = prefix.trimmed();

    // Case-insensitive exact match
    if (bc.compare(px, Qt::CaseInsensitive) == 0)
        return true;

    // Case-insensitive prefix + '-'
    if (bc.startsWith(px + "-", Qt::CaseInsensitive))
        return true;

    return false;
}

// ------------------------------------------------------------
//  MaterialFamilyDetector – teljes, jövőbiztos verzió
// ------------------------------------------------------------
class MaterialFamilyDetector {
public:
    static MaterialFamily detect_fromBarcode(const QString& bc) {

        // --- TOK / TOKFED ---
        if (matchPrefix(bc, "NP-TF")) return MaterialFamily::TokFed;
        if (matchPrefix(bc, "NP-T"))  return MaterialFamily::Tok;

        // --- ZÁRÓ ---
        if (matchPrefix(bc, "NP-CZ")) return MaterialFamily::Zaro;
        if (matchPrefix(bc, "NP-SZ")) return MaterialFamily::Zaro;
        if (matchPrefix(bc, "SR-Z"))  return MaterialFamily::Zaro;
        if (matchPrefix(bc, "MT-Z"))  return MaterialFamily::Zaro;
        if (matchPrefix(bc, "ROL-GYZ")) return MaterialFamily::Zaro;
        if (matchPrefix(bc, "ROL-EZ"))  return MaterialFamily::Zaro;

        // --- LÁB ---
        if (matchPrefix(bc, "NP-CL")) return MaterialFamily::Lab;
        if (matchPrefix(bc, "NP-SL")) return MaterialFamily::Lab;
        if (matchPrefix(bc, "MT-L"))  return MaterialFamily::Lab;

        // --- PÁLCA ---
        if (matchPrefix(bc, "SP"))      return MaterialFamily::Palca;
        if (matchPrefix(bc, "MT-P"))    return MaterialFamily::Palca;
        if (matchPrefix(bc, "ROL-P"))   return MaterialFamily::Palca;

        // --- TENGELY ---
        if (matchPrefix(bc, "TE"))         return MaterialFamily::Tengely;
        if (matchPrefix(bc, "NP-ROLL"))    return MaterialFamily::Tengely;

        // --- SÚLY ---
        if (matchPrefix(bc, "NP-BAR")) return MaterialFamily::Suly;

        // --- MOTOR ---
        if (matchPrefix(bc, "NP-MOT")) return MaterialFamily::Motor;

        // --- HAJTÓELEM ---
        if (matchPrefix(bc, "NP-HK")) return MaterialFamily::HajtoElem;

        // --- RUGÓ ---
        if (matchPrefix(bc, "ROL-RRG")) return MaterialFamily::Rugo;
        if (matchPrefix(bc, "ROL-TRG")) return MaterialFamily::Rugo;

        // --- FITTING ---
        if (matchPrefix(bc, "MT-VE"))      return MaterialFamily::Fitting;
        if (matchPrefix(bc, "SR-KONZOL"))  return MaterialFamily::Fitting;
        if (matchPrefix(bc, "MT-PVD"))     return MaterialFamily::Fitting;
        if (matchPrefix(bc, "ROL-PD"))     return MaterialFamily::Fitting;
        if (matchPrefix(bc, "ROL-TEV"))    return MaterialFamily::Fitting;

        // --- TOKVÉG / POFA ---
        if (matchPrefix(bc, "NP-TP")) return MaterialFamily::TokVeg;

        // --- CSAVAR ---
        if (matchPrefix(bc, "NP-CSAV")) return MaterialFamily::Csavar;

        // --- KIEGÉSZÍTŐ ---
        if (matchPrefix(bc, "NP-CLB"))  return MaterialFamily::Kiegeszito;
        if (matchPrefix(bc, "NP-CLBR")) return MaterialFamily::Kiegeszito;

        // --- TEXTIL ---
        if (matchPrefix(bc, "ROL-FAB")) return MaterialFamily::Textil;

        return MaterialFamily::Unknown;
    }
};
