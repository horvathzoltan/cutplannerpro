// #pragma once
// #include <QString>

// static inline bool matchPrefix(const QString& barcode, const QString& prefix)
// {
//     const QString bc = barcode.trimmed();
//     const QString px = prefix.trimmed();

//     // Case-insensitive pontos egyezés
//     if (bc.compare(px, Qt::CaseInsensitive) == 0)
//         return true;

//     // Case-insensitive prefix + '-' egyezés
//     if (bc.startsWith(px + "-", Qt::CaseInsensitive))
//         return true;

//     return false;
// }

