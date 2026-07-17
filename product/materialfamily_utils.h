// #pragma once

// #include <QString>




// namespace MaterialFamilyUtils{

// // ------------------------------------------------------------
// //  Prefix matcher (case-insensitive, exact or prefix + '-')
// // ------------------------------------------------------------


// static inline bool matchPrefix(const QString& barcode, const QString& prefix)
// {
//     QString bc = barcode.trimmed();
//     QString px = prefix.trimmed();

//     // ha '*' van a végén → prefix-család
//     bool wildcard = px.endsWith("*");
//     if (wildcard)
//         px.chop(1);   // NP-CL* → NP-CL

//     // exact match
//     if (bc.compare(px, Qt::CaseInsensitive) == 0)
//         return true;

//     // wildcard esetén: csak "px-" kezdetű lehet
//     if (wildcard) {
//         if (bc.startsWith(px + "-", Qt::CaseInsensitive))
//             return true;
//         return false;
//     }

//     // normál prefix → NINCS wildcard → NINCS prefix-match
//     // csak exact match engedélyezett
//     return false;
// }

// }