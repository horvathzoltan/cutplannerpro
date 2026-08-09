#pragma once

#include <QString>



namespace Cutting{
namespace Plan{

enum class Source {
    Stock,     // 🧱 Normál profilkészlet
    Reusable   // ♻️ Hulladékból újravágás
};


namespace SourceUtils{

inline QString toDisplayText(Source s){
    switch(s){
        case Source::Stock: return "Stock";
        case Source::Reusable: return "Reusable";
        default: return "Unknown";
        };
}

inline QString toCsv(Source s) {
    switch (s) {
    case Source::Stock:    return "STOCK";
    case Source::Reusable: return "REUSABLE";
    }
    return "STOCK";
}

inline Source fromCsv(const QString& s) {
    if (s.compare("STOCK", Qt::CaseInsensitive) == 0) return Source::Stock;
    if (s.compare("REUSABLE", Qt::CaseInsensitive) == 0) return Source::Reusable;
    return Source::Stock;
}

} // end namespace SourceUtils

} //end namespace Plan
} // end namespace Cutting
