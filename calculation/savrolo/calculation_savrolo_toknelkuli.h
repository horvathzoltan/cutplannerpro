#pragma once

#include "calculation/sizecalcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Savrolo{
namespace TokNelkuli{
namespace GyartasiMeret{

inline double calcFelsoSin(double width){
    return width - 16;
}

inline double calcTengely(double width){
    return width - 33;
}

inline double calcVaszon(double width){
    return width - 40;
}

inline double calcZaro(double width){
    return width - 30;
}


}

/*
SR;TN;FelsoSin;SR-FSN*
SR;TN;Tengely;TE-H-32*
SR;TN;Zaro;SR-Z+P*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  SizeCalcMode mode)
{

    // --- TOK ---
    if (role == "RSR-FSN") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcFelsoSin(width);
    }

    // --- TENGELY ---
    if (role == "RTE-H-32") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RSR-VASZON") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- ZARO ---
    if (role == "RSR-ZP") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcZaro(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen SR-TN role: " + role);
    return std::nullopt;
}
}}}
