#pragma once

#include "calculation/calcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Savrolo{
namespace MiniTokos{
namespace GyartasiMeret{

inline double calcTok(double width){
    return width - 35;
}

inline double calcTengely(double width){
    return width - 35;
}

inline double calcVaszon(double width){
    return width - 45;
}

inline double calcZaro(double width){
    return width - 45;
}

inline double calcLab(double width){
    return width - 45;
}
}

/*
SR;MINIT;Tok;MT-T*
SR;MINIT;Tengely;TE-H-18*
SR;MINIT;Zaro;MT-Z+P*
SR;MINIT;Lab;MT-L2*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  CalcMode mode)
{

    // --- TOK ---
    if (role == "MT-T") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTok(width);
    }

    // --- TENGELY ---
    if (role == "TE-H") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "SR-VASZON") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- ZARO ---
    if (role == "MT-Z+P") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcZaro(width);
    }

    // --- LAB ---
    if (role == "MT-L2") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcLab(height);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen MT-MINIT role: " + role);
    return std::nullopt;
}
}}}
