#pragma once

#include <QString>


enum class SortMode {
    BySize,        // méret szerint csökkenő
    ByMaterial,    // anyag szerint
    ByWorkflow     // tok → záró → súly → vászon → tengely → láb
};


namespace SortModeUtils{

inline QString toDisplay(SortMode mode){
    if(mode == SortMode::BySize) return "Méret szerint (desc)";
    if(mode == SortMode::ByMaterial) return "Anyag szerint";
    if(mode == SortMode::ByWorkflow) return "Workflow sorrend";
    return "Ismeretlen";
}

inline QString toCsv(SortMode mode){
    if(mode == SortMode::BySize) return "BySize";
    if(mode == SortMode::ByMaterial) return "ByMaterial";
    if(mode == SortMode::ByWorkflow) return "ByWorkflow";
    return "Unknown";
}

inline SortMode fromCsv(const QString& s){
    QString t = s.trimmed().toLower();

    if(t == "bysize") return SortMode::BySize;
    if(t == "bymaterial") return SortMode::ByMaterial;
    if(t == "byworkflow") return SortMode::ByWorkflow;
    return SortMode::BySize; // default
}
}