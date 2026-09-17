#pragma once

#include "calculation/calcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Roletta{
namespace Ujgyongyos{
namespace GyartasiMeret{
inline double calcTengely(double width){
    return width - 35;
}

inline double calcVaszon(double width){
    return width - 45;
}

inline double calcZaro(double width){
    return width - 45;
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
                                  CalcMode mode)
{
    // --- TENGELY ---
    if (role == "RTE-H-32") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RNP-VASZON") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- ZÁRÓ ---
    if (role == "RROL-GYZ") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcZaro(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-UJGY role: " + role);
    return std::nullopt;
}
}}}
