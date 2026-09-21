#pragma once

#include "calculation/sizecalcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Roletta{
namespace Tetoteri{
namespace GyartasiMeret{
inline double calcTengely(double width){
    return width - 25;
}

inline double calcVaszon(double width){
    return width - 27;
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
                                  SizeCalcMode mode)
{
    // --- TENGELY ---
    if (role == "RTE-R-23") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RNP-VASZON") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- AlsóPálca ---
    if (role == "RROL-P") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcAlsoPalca(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-TET_NAGY role: " + role);
    return std::nullopt;
}
}}}
