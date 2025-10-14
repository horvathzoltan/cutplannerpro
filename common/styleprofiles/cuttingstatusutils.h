#pragma once
#include <QString>
#include <QColor>
#include "model/cutting/instruction/cutinstruction.h"
#include "common/styleprofiles/cuttingcolors.h"

/**
 * @brief Helper függvények a CutStatus enumhoz
 *
 * Ezeket a RowGenerator és a TableManager is használhatja,
 * így nem kell mindenhol switch-case vagy if-else logikát duplikálni.
 */

namespace CuttingStatusUtils {
inline QString toText(CutStatus st) {
    switch (st) {
    case CutStatus::Pending:    return "Pending";
    case CutStatus::InProgress: return "In Progress";
    case CutStatus::Done:       return "Done";
    case CutStatus::Error:      return "Error";
    }
    return "?";
}

inline QColor toColor(CutStatus st) {
    switch (st) {
    case CutStatus::Pending:    return CuttingColors::Pending;
    case CutStatus::InProgress: return CuttingColors::InProgress;
    case CutStatus::Done:       return CuttingColors::Done;
    case CutStatus::Error:      return CuttingColors::Error;
    }
    return CuttingColors::DefaultFg;
}
}   // namespace CuttingStatusUtils
