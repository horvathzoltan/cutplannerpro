#pragma once

#include "calculation/calcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Roletta{
namespace TetoteriNagyKonzolos{
namespace GyartasiMeret{
inline double calcTengely(double width){
    return width - 30;
}

inline double calcVaszon(double width){
    return width - 40;
}

inline double calcAlsoPalca(double width){
    return width - 27;
}



}

/*
ROL;TET_NAGY;Tengely;TE-R-23*
ROL;TET_NAGY;Palca;ROL-P*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  CalcMode mode)
{
    // --- TENGELY ---
    if (role == "RTE-R-23") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RNP-VASZON") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- AlsóPálca ---
    if (role == "RROL-P") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcAlsoPalca(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-TET_NAGY role: " + role);
    return std::nullopt;
}
}}}
