// #pragma once
// #include <QVector>
// #include <QHash>
// #include "leftover/model/leftoverstockentry.h"

// struct LeftoverSubstitutionResult {
//     QVector<LeftoverStockEntry> targets;
//     QHash<QUuid, QUuid> mapping;   // target → substitute
//     QVector<LeftoverStockEntry> substitutesUsed;
//     QVector<LeftoverStockEntry> substitutesMissing;
// };

// class LeftoverSubstitutionEngine {
// public:
//     static LeftoverSubstitutionResult run(
//         const QVector<LeftoverStockEntry>& targets,
//         int daysThreshold,
//         int tolerance_mm);
// };
