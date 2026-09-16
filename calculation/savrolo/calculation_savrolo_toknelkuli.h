#pragma once

#include "calculation/calcmode.h"
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
                                  CalcMode mode)
{

    // --- TOK ---
    if (role == "RSR-FSN") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcFelsoSin(width);
    }

    // --- TENGELY ---
    if (role == "RTE-H-32") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RSR-VASZON") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- ZARO ---
    if (role == "RSR-ZP") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcZaro(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen SR-TN role: " + role);
    return std::nullopt;
}
}}}
