#pragma once

#include "calculation/sizecalcmode.h"
#include "calculation/roletta/calculation_roletta_ujgyongyos.h"
#include "calculation/roletta/calculation_roletta_alap.h"
#include "calculation/roletta/calculation_roletta_tetoterinagy.h"
#include "calculation/roletta/calculation_roletta_tokozottsines.h"


#include "common/logger.h"
#include <QMap>
#include <QString>

// motor: SmartHome Smart 45E-10: tengely 111 helyett 109

namespace Calculation{
namespace Roletta{

inline std::optional<double> calc(const QString& subtype,
                                  const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  SizeCalcMode mode){

    if(subtype == "ALAP"){
        return Alap::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "RUG"){
        //return Rugos::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "TET"){
        //return Tetoteri::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "TET_NAGY"){
        return TetoteriNagyKonzolos::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "TSIN"){
       return TokozottSines::calc(attributes, role, width, height, mode);
    }
    else if(subtype == "UJGY"){
        return Ujgyongyos::calc(attributes, role, width, height, mode);
    }
    else {
        zInfo("Ismeretlen altípus:"+subtype);
    }

    return std::nullopt;
}

}} // endof
