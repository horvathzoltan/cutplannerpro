#pragma once

#include "calculation/sizecalcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Savrolo{
namespace Tokozott{
namespace GyartasiMeret{

inline double calcTok(double width){
    return width -6;
}

inline double calcTengely(double width){
    return width -23;
}

inline double calcVaszon(double width){
    return width -30;
}

inline double calcZaro(double width){
    return width -20;
}

}

/*
SR;T;Tok;SR-T*
SR;T;Tengely;TE-H-32*
SR;T;Zaro;SR-Z+P*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  SizeCalcMode mode)
{

    // --- TOK ---
    if (role == "RSR-T") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTok(width);
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
    zInfo("Ismeretlen SR-T role: " + role);
    return std::nullopt;
}
}}}
