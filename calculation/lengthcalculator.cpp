#include "lengthcalculator.h"
#include "calculation/naphalo/calculatuon_naphalo.h"

#include "common/logger.h"

// =========================
// FŐ API
// =========================

std::optional<double> LengthCalculator::calculate(
    const QString& type,
    const QString& subtype,
    const QMap<QString, QString>& attributes,
    const QString& role,
    double width,
    double height,
    CalcMode mode)
{
    // 2) Típus/altípus specializáció
    if (type == "NP") {
        return Calculation::Naphalo::calc(subtype, attributes, role, width, height, mode);
    }
    else if (type == "SR") {
        //return Calculation::Savrolo::calc(subtype, attributes, role, width, height, mode);
    }
    else if (type == "ROL") {
        //return Calculation::Roletta::calc(subtype, attributes, role, width, height, mode);
    } else {
        zInfo("Ismeretlen típus:"+type);
    }

    return std::nullopt;
}


std::optional<double> LengthCalculator::compensate(
    const QString& type,
    const QString& subtype,
    const QMap<QString, QString>& attributes,
    const QString& role)
{
    // 2) Típus/altípus specializáció
    if (type == "NP") {
        return Calculation::Naphalo::compensation(subtype, attributes, role);
    }
    else if (type == "SR") {
        //return Calculation::Savrolo::compensation(subtype, attributes, role, width, height, mode);
    }
    else if (type == "ROL") {
        //return Calculation::Roletta::compensation(subtype, attributes, role, width, height, mode);
    } else {
        zInfo("Ismeretlen típus:"+type);
    }

    return std::nullopt;
}


