#pragma once

#include "calculation/sizecalcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Roletta{
namespace TokozottSines{
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

inline double calcLab(double height){
    return height;
}
}

namespace UvegMeret{

inline double calcTok(double width){
    return width +10;
}

inline double calcTengely(double width){
    return width +5;
}

inline double calcVaszon(double width){
    return width - 5;
}

inline double calcZaro(double width){
    return width - 7;
}

inline double calcLab(double height){
    return height;
}
}
/*
SR;MINIT;Tok;MT-T*
SR;MINIT;Tengely;TE-H-18*
SR;MINIT;Zaro;MT-Z*
SR;MINIT;Lab;MT-L2*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  SizeCalcMode mode)
{

    // --- TOK ---
    if (role == "RMT-T") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTok(width);
        if(mode == SizeCalcMode::Uveg)
            return UvegMeret::calcTok(width);
    }

    // --- TENGELY ---
    if (role == "RTE-H-18") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTengely(width);
        if(mode == SizeCalcMode::Uveg)
            return UvegMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RSR-VASZON") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcVaszon(width);
        if(mode == SizeCalcMode::Uveg)
            return UvegMeret::calcVaszon(width);
    }

    // --- ZARO ---
    if (role == "RMT-Z") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcZaro(width);
        if(mode == SizeCalcMode::Uveg)
            return UvegMeret::calcZaro(width);
    }

    // ha kell festeni, a láb 4 cm-el hosszabb, és fúrni is kell a felfüggesztés miatt
    // --- LAB ---
    if (role == "RMT-L") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcLab(height);
        if(mode == SizeCalcMode::Uveg)
            return UvegMeret::calcLab(height);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen ROL-TS role: " + role);
    return std::nullopt;
}
}}}
