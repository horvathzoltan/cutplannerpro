#pragma once

#include "calculation/sizecalcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Roletta{
namespace Motoros{
namespace TeljesMeret{
inline double calcTengely(double width){
    return width - 55;
}

inline double calcVaszon(double width){
    return width - 65;
}

inline double calcZaro(double width){
    return width - 65;
}

}

/*
ROL;UJGY;Tengely;TE-R-32*
ROL;UJGY;Zaro;ROL-GYZ*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  SizeCalcMode mode)
{
    // --- TENGELY ---
    if (role == "RTE-B-47") {
        if(mode == SizeCalcMode::Teljes)
            return TeljesMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RNP-VASZON") {
        if(mode == SizeCalcMode::Teljes)
            return TeljesMeret::calcVaszon(width);
    }

    // --- ZÁRÓ ---
    if (role == "RROL-EZ") {
        if(mode == SizeCalcMode::Teljes)
            return TeljesMeret::calcZaro(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-UJGY role: " + role);
    return std::nullopt;
}
}}}
