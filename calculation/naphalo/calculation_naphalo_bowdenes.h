#pragma once

#include "calculation/sizecalcmode.h"
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
    return width - 44;
}

inline double calcTengely_Kurblis(double width){
    return width - 52;
}

inline double calcZaroSuly(double width){
    return width - 85;
}

inline double calcVaszon(double width){
    return width - 55;
}
} // endof GyartasiMeret

inline std::optional<double> calc(const QMap<QString, QString>& attributes,
                                  const QString& role,
                                  double width,
                                  double height,
                                  SizeCalcMode mode)
{
    // --- TOK ---
    if (role == "RNP-T") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTok(width);
    }

    // --- TOKFEDEL ---
    if (role == "RNP-TF") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcTokFedel(width);
    }

    // --- ZÁRÓ ---
    if (role == "RNP-SZ") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcZaro(width);
    }

    // --- BOWDEN ---
    if (role == "RNP-B")
    {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcBowden(height);
    }

    // --- TENGELY ---
    if (role == "RNP-ROLL") {
        // attribútumfüggő tengely
        const QString meghajtas_key = "meghajtas";
        if(attributes.contains(meghajtas_key))
        {
            QString meghajtas = attributes.value(meghajtas_key);
            if (meghajtas == "motoros") {
                if(mode == SizeCalcMode::Gyartasi)
                    return GyartasiMeret::calcTengely_Motoros(width);
            }
            else if (meghajtas == "kurblis") {
                if(mode == SizeCalcMode::Gyartasi)
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
    if (role == "RNP-SBAR") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcZaroSuly(width);
    }

    // --- VÁSZON ---
    if (role == "RNP-VASZON") {
        if(mode == SizeCalcMode::Gyartasi)
            return GyartasiMeret::calcVaszon(width);
    }

    // --- ISMERETLEN ROLE ---
    zInfo("Ismeretlen NP-BOW role: " + role);
    return std::nullopt;
}

std::optional<double> compensation(const QMap<QString, QString>& attributes,
                                   const QString& role){
    if (role == "RNP-ROLL") {
        // attribútumfüggő tengely
        const QString meghajtas_key = "meghajtas";
        if(attributes.contains(meghajtas_key))
        {
            QString meghajtas = attributes.value(meghajtas_key);
            if (meghajtas == "motoros") {
                return 0;//-4;
            }
            else if (meghajtas == "kurblis") {
                return 0;//-3;
            }
            else {
                zInfo("Ismeretlen attr value: "+meghajtas);
            }
        }
        else{
            zInfo(L("Ismeretlen attr key: ")+meghajtas_key);
        }
    }

    // --- TOK ---
    if (role == "RNP-TOK") {
        return -0.50;
    }

    // --- SÚLY ---
    if (role == "RNP-BAR") {
        return 0;//-10;
    }

    return std::nullopt;
}

} //end of namespace Cipzaras
}
}
