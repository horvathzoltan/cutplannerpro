#pragma once

#include "calculation/calcmode.h"
#include "calculation/naphalo/calculation_naphalo_cipzaras.h"
#include "calculation/naphalo/calculation_naphalo_sines.h"
#include "calculation/naphalo/calculation_naphalo_bowdenes.h"

#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Naphalo{

inline std::optional<double> calc(const QString& subtype,
                                  const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  CalcMode mode){

    if(subtype == "CIP"){
        return Cipzaras::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "SIN"){
        return Sines::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "BOW"){
        return Bowdenes::calc(attributes, role, width, height, mode);
    }
    else {
        zInfo("Ismeretlen altípus:"+subtype);
    }

    return std::nullopt;
}

inline std::optional<double> compensation(const QString& subtype,
                                  const QMap<QString, QString>& attributes,
                                  const QString& role){

    if(subtype == "CIP"){
        return Cipzaras::compensation(attributes, role);
    }
    else if(subtype == "SIN"){
        return Sines::compensation(attributes, role);
    }
    else if(subtype == "BOW"){
        return Bowdenes::compensation(attributes, role);
    }
    else {
        zInfo("Ismeretlen altípus:"+subtype);
    }

    return std::nullopt;
}
} // end of namespace Naphalo
} // end of namespace Calculation