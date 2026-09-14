#pragma once

#include "calculation/calcmode.h"
#include "calculation/savrolo/calculation_savrolo_toknelkuli.h"
#include "calculation/savrolo/calculation_savrolo_minitokos.h"
#include "calculation/savrolo/calculation_savrolo_tokozott.h"

#include "common/logger.h"
#include <QMap>
#include <QString>

// motor: SmartHome Smart 45E-10: tengely 111 helyett 109

namespace Calculation{
namespace Savrolo{

inline std::optional<double> calc(const QString& subtype,
                                  const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  CalcMode mode){

    if(subtype == "T"){
        return Tokozott::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "TN"){
        return TokNelkuli::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "MINIT"){
        return MiniTokos::calc(attributes, role, width, height, mode);
    }

    else {
        zInfo("Ismeretlen altípus:"+subtype);
    }

    return std::nullopt;
}

}} // endof
