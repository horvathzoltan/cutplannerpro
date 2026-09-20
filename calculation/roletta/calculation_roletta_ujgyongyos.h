#pragma once

#include "calculation/sizecalcmode.h"
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
                                  SizeCalcMode mode)
{
    // --- TENGELY ---
    if (role == "RTE-H-32") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RNP-VASZON") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- ZÁRÓ ---
    if (role == "RROL-GYZ") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcZaro(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-UJGY role: " + role);
    return std::nullopt;
}
}}}
