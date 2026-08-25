#pragma once
#include <QUuid>
#include <QString>


enum class MaterialKind
{
    Simple,   ///< Egyszerű, egykomponensű anyag
    Bundle  ///< Compound anyag (bundle)
};


namespace MaterialKindUtils{
    inline QString toString(MaterialKind k){
        if(k == MaterialKind::Simple) return "Simple";
        if(k == MaterialKind::Bundle) return "Bundle";
        return "Unknown";
    }

    inline MaterialKind fromString(const QString& str){
        QString v = str.trimmed().toLower();
        if(v == "simple") return MaterialKind::Simple;
        if(v== "bundle") return MaterialKind::Bundle;
        return MaterialKind::Simple;
    }
}
