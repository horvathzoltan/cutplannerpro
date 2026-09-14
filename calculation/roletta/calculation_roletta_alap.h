#pragma once

#include "calculation/calcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Roletta{
namespace Alap{
namespace GyartasiMeret{
inline double calcTengely(double width){
    return width - 35;
}

inline double calcVaszon(double width){
    return width - 45;
}

inline double calcAlsoPalca(double width){
    return width - 45;
}

}

/*
ROL;ALAP;Tengely;TE-R-23*
ROL;ALAP;Palca;ROL-P*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  CalcMode mode)
{
    // --- TENGELY ---
    if (role == "TE-R") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "NP-VASZON") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- AlsóPálca ---
    if (role == "ROL-P") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcAlsoPalca(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-ALAP role: " + role);
    return std::nullopt;
}
}}}
