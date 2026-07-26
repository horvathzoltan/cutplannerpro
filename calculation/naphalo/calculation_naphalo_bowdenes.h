#pragma once

#include "calculation/calcmode.h"
#include "common/logger.h"
#include <QMap>
#include <QString>

namespace Calculation{
namespace Naphalo{
namespace Bowdenes{
namespace GyartasiMeret{
inline double calcTok(double width){
    return width - 5;
}

inline double calcTokFedel(double width){
    return width - 5;
}

inline double calcBowden(double height){
    return height +25;
}

inline double calcZaro(double width){
    return width - 40;
}

inline double calcTengely_Motoros(double width){
    return width - 45;
}

inline double calcTengely_Kurblis(double width){
    return width - 54;
}

inline double calcZaroSuly(double width){
    return width - 75;
}

inline double calcVaszon(double width){
    return width - 55;
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
    if (role == "NP-SZ") {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcZaro(width);
    }

    // --- BOWDEN ---
    if (role == "NP-B")
    {
        if(mode == CalcMode::GyartasiMeret)
            return GyartasiMeret::calcBowden(height);
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
    zInfo("Ismeretlen NP-BOW role: " + role);
    return std::nullopt;
}

std::optional<double> compensation(const QMap<QString, QString>& attributes,
                                   const QString& role){
    if (role == "NP-ROLL") {
        // attribútumfüggő tengely
        const QString meghajtas_key = "meghajtas";
        if(attributes.contains(meghajtas_key))
        {
            QString meghajtas = attributes.value(meghajtas_key);
            if (meghajtas == "motoros") {
                return -4;
            }
            else if (meghajtas == "kurblis") {
                return -2;
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
        return -10;
    }

    return std::nullopt;
}

} //end of namespace Cipzaras
}
}