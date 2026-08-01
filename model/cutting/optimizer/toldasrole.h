#pragma once

#include <QString>


enum class ToldasRole {
    None,
    Main,
    Toldat
};

namespace ToldasRoleUtils{
    inline QString toString(ToldasRole r){
        if(r==ToldasRole::None) return "";
        if(r==ToldasRole::Main) return "TOLDAS_MAIN";
        if(r==ToldasRole::Toldat) return "TOLDAS_TOLDAT";
        return "";
    }
} //endof namespace ToldasRoleUtils