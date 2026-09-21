#ifndef MOTOROS_H
#define MOTOROS_H

#endif // MOTOROS_H
#pragma once

#include "calculation/sizecalcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Savrolo{
namespace Motoros{
namespace GyartasiMeret{

inline double calcTok(double width){
    return width -6;
}

inline double calcTengely(double width){
    return width -30;
}

inline double calcVaszon(double width){
    return width -37;
}

inline double calcZaro(double width){
    return width -27;
}
} //endof namespace

namespace VaszonMeret{

inline double calcTok(double width){
    return width +31;
}

inline double calcTengely(double width){
    return width +7;
}

inline double calcVaszon(double width){
    return width;
}

inline double calcZaro(double width){
    return width +10;
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
        if(mode == SizeCalcMode::Vaszon)
            return VaszonMeret::calcTok(width);
    }

    // --- TENGELY ---
    if (role == "RTE-B-47") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTengely(width);
        if(mode == SizeCalcMode::Vaszon)
            return VaszonMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RSR-VASZON") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcVaszon(width);
        if(mode == SizeCalcMode::Vaszon)
            return VaszonMeret::calcVaszon(width);
    }

    // --- ZARO ---
    if (role == "RSR-ZP") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcZaro(width);
        if(mode == SizeCalcMode::Vaszon)
            return VaszonMeret::calcZaro(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen SR-T role: " + role);
    return std::nullopt;
}
}}}
