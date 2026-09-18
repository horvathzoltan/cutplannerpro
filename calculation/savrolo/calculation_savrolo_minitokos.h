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
SR;MINIT;Zaro;MT-Z+P*
SR;MINIT;Lab;MT-L2*
*/

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  CalcMode mode)
{

    // attribútumfüggő tengely
    const QString szamitas_key = "szamitas";
    if(attributes.contains(szamitas_key))
    {
        QString szamitas = attributes.value(szamitas_key);
        if (szamitas == "uvegmeret") {
            mode = CalcMode::UvegMeret;

        }
        else if (szamitas == "gyartasimeret") {
            mode = CalcMode::GyartasiMeret;

        }
        else {
            mode = CalcMode::Unknown;
        }
    }

    // --- TOK ---
    if (role == "RMT-T") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTok(width);
        else if(mode == CalcMode::UvegMeret)
            return UvegMeret::calcTok(width);
    }

    // --- TENGELY ---
    if (role == "RTE-H-18") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTengely(width);
        else if(mode == CalcMode::UvegMeret)
            return UvegMeret::calcTengely(width);
    }

    // --- VÁSZON ---
    if (role == "RSR-VASZON") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcVaszon(width);
        else if(mode == CalcMode::UvegMeret)
            return UvegMeret::calcVaszon(width);
    }

    // --- ZARO ---
    if (role == "RMT-ZP") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcZaro(width);
        else if(mode == CalcMode::UvegMeret)
            return UvegMeret::calcZaro(width);
    }

    // --- LAB ---
    if (role == "RMT-L2") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcLab(height);
        if(mode == CalcMode::UvegMeret)
            return UvegMeret::calcLab(height);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen SR-MINIT role: " + role);
    return std::nullopt;
}
}}}
