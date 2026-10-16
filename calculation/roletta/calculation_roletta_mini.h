#pragma once

#include "calculation/sizecalcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Roletta{
namespace Mini{
namespace TeljesMeret{
inline double calcTengely(double width){
    return width - 30;
}

inline double calcVaszon(double width){
    return width - 40;
}

inline double calcZaro(double width){
    return width - 40;
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
    if (role == "RTE-S-18") {
        if(mode == SizeCalcMode::Teljes)
            return TeljesMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RNP-VASZON") {
        if(mode == SizeCalcMode::Teljes)
            return TeljesMeret::calcVaszon(width);
    }

    // --- ZÁRÓ ---
    if (role == "RROL-P") {
        if(mode == SizeCalcMode::Teljes)
            return TeljesMeret::calcZaro(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-UJGY role: " + role);
    return std::nullopt;
}
}}}
