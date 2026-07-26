#pragma once

#include <QString>


enum class CalcMode {
    GyartasiMeret
};

namespace CalcModeUtils{

inline QString toString(CalcMode m){
    if(m==CalcMode::GyartasiMeret) return "GyartasiMeret";
    return "";
}

}