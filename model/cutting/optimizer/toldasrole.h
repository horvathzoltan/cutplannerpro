#pragma once

#include <QString>


enum class ToldasRole {
    None,
    Main,
    Toldat
};

namespace ToldasRoleUtils{

inline QString toDisplayText(ToldasRole r){
    switch(r){
    case ToldasRole::None:   return "Nincs toldás";
    case ToldasRole::Main:   return "Fődarab";
    case ToldasRole::Toldat: return "Toldat";
    }
    return "Ismeretlen";
}

    inline QString toCsv(ToldasRole r){
        if(r==ToldasRole::None) return "";
        if(r==ToldasRole::Main) return "TOLDAS_MAIN";
        if(r==ToldasRole::Toldat) return "TOLDAS_TOLDAT";
        return "";
    }

    inline ToldasRole fromCsv(const QString& s){
        if(s.compare("TOLDAS_MAIN", Qt::CaseInsensitive) == 0) return ToldasRole::Main;
        if(s.compare("TOLDAS_TOLDAT", Qt::CaseInsensitive) == 0) return ToldasRole::Toldat;
        return ToldasRole::None;
    }
} //endof namespace ToldasRoleUtils