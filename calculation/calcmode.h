#pragma once

#include <QString>


enum class CalcMode {
    GyartasiMeret, UvegMeret, Unknown
};

namespace CalcModeUtils{

inline QString toString(CalcMode m){
    if(m==CalcMode::GyartasiMeret) return "GyartasiMeret";
    if(m==CalcMode::UvegMeret) return "UvegMeret";
    return "";
}

}
