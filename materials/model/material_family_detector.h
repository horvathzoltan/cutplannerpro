// #pragma once
// #include <QString>
// #include "materials/model/material_family.h"

// // ------------------------------------------------------------
// //  Prefix matcher (case-insensitive, exact or prefix + '-')
// // ------------------------------------------------------------
// static inline bool matchPrefix(const QString& barcode, const QString& prefix)
// {
//     const QString bc = barcode.trimmed();
//     const QString px = prefix.trimmed();

//     // Case-insensitive exact match
//     if (bc.compare(px, Qt::CaseInsensitive) == 0)
//         return true;

//     // Case-insensitive prefix + '-'
//     if (bc.startsWith(px + "-", Qt::CaseInsensitive))
//         return true;

//     return false;
// }

