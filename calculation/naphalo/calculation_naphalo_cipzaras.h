#pragma once

#include "calculation/calcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Naphalo{
namespace Cipzaras{
namespace GyartasiMeret{
inline double calcTok(double width){
    return width - 30;
}

inline double calcTokFedel(double width){
    return width - 5;
}

inline double calcLab(double height){
    return height - 103;
}

inline double calcZaro(double width){
    return width - 123;
}

inline double calcLabBetet(double height){
    return height - 103;
}

inline double calcTengely_Motoros(double width){
    return width - 111;//115;
}

inline double calcTengely_Kurblis(double width){
    return width - 115;
}

inline double calcZaroSuly(double width){
    return width - 175;
}

inline double calcVaszon(double width){
    return width - 82;
}
} // endof GyartasiMeret

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  CalcMode mode)
{
    // --- TOK ---
    if (role == "NP-T") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTok(width);
    }

    // --- TOKFEDEL ---
    if (role == "NP-TF") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcTokFedel(width);
    }

    // --- ZÁRÓ ---
    if (role == "NP-CZ") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcZaro(width);
    }

    // --- LAB ---
    if (role == "NP-CL" ||
        role == "NP-CLB" ||
        role == "NP-CLT" ||
        role == "NP-CL2+CLT2+CLB2")
    {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcLab(height);
    }

    // --- TENGELY ---
    if (role == "NP-ROLL") {
        // attribútumfüggő tengely
        const QString meghajtas_key = "meghajtas";
        if(attributes.contains(meghajtas_key))
        {
            QString meghajtas = attributes.value(meghajtas_key);
            if (meghajtas == "motoros") {
                if(mode == CalcMode::GyartasiMeret)
                    return GyartasiMeret::calcTengely_Motoros(width);
            }
            else if (meghajtas == "kurblis") {
                if(mode == CalcMode::GyartasiMeret)
                    return GyartasiMeret::calcTengely_Kurblis(width);
            }
            else {
                zInfo("Ismeretlen attr value: "+meghajtas);
            }
        }
        else{
            zInfo(L("Ismeretlen attr key: ")+meghajtas_key);
        }
    }

    // --- SÚLY ---
    if (role == "NP-BAR") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcZaroSuly(width);
    }

    // --- VÁSZON ---
    if (role == "NP-VASZON") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen NP-CIP role: " + role);
    return std::nullopt;
}

std::optional<double> compensation(
    const QMap<QString, QString>& attributes,
    const QString& role){

    if (role == "NP-ROLL") {
        // attribútumfüggő tengely
        const QString meghajtas_key = "meghajtas";
        if(attributes.contains(meghajtas_key))
        {
            QString meghajtas = attributes.value(meghajtas_key);
            if (meghajtas == "motoros") {
                return 0;//+4;
            }
            else if (meghajtas == "kurblis") {
                return 0;
            }
            else {
                zInfo("Ismeretlen attr value: "+meghajtas);
            }
        }
        else{
            zInfo(L("Ismeretlen attr key: ")+meghajtas_key);
        }
    }

    // --- SÚLY ---
    if (role == "NP-BAR") {
        return 0;//-10;
    }

    return std::nullopt;
}

} //end of namespace Cipzaras
}
}
