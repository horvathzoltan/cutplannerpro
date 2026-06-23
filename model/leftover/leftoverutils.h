#pragma once
#include <QString>

namespace LeftoverUtils {

// -----------------------------
//  Prefix prioritás
// -----------------------------
inline int prefixRank(const QString& code) {
    QString p = code.left(3).toUpper();

    if (p == "RSM") return 0;   // Manual leftover → legelső
    if (p == "RST") return 1;   // Optimization leftover → második

    return 2;                   // minden más prefix → a végére
}

// -----------------------------
//  Postfix (szám) kinyerése
// -----------------------------
inline int postfixValue(const QString& code) {
    int lastDash = code.lastIndexOf('-');
    if (lastDash < 0)
        return -1;

    bool ok = false;
    int v = code.mid(lastDash + 1).toInt(&ok);
    return ok ? v : -1;
}

// -----------------------------
//  A végső comparator
// -----------------------------
inline bool leftoverLess(const QString& a, const QString& b) {

    // 1) Elsődleges kulcs: postfix (szám)
    int pa = postfixValue(a);
    int pb = postfixValue(b);

    if (pa != pb)
        return pa < pb;

    // 2) Másodlagos kulcs: prefix prioritás
    int ra = prefixRank(a);
    int rb = prefixRank(b);

    if (ra != rb)
        return ra < rb;

    // 3) Harmadlagos kulcs: teljes lexikografikus összehasonlítás
    return a < b;
}

} // namespace LeftoverUtils
