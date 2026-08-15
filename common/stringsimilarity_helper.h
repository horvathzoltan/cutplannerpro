#pragma once
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

namespace StringSimilarity {

// --- 1) Levenshtein-távolság ---
inline int levenshtein(const QString& a, const QString& b)
{
    const int n = a.size();
    const int m = b.size();
    if (n == 0) return m;
    if (m == 0) return n;

    QVector<int> prev(m + 1), curr(m + 1);

    for (int j = 0; j <= m; ++j)
        prev[j] = j;

    for (int i = 1; i <= n; ++i) {
        curr[0] = i;
        for (int j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] = std::min({ prev[j] + 1,
                                curr[j - 1] + 1,
                                prev[j - 1] + cost });
        }
        prev = curr;
    }
    return curr[m];
}


// --- 2) Prefix similarity (80% egyezés) ---
inline bool prefixSimilar(const QString& a, const QString& b)
{
    int minLen = std::min(a.size(), b.size());
    if (minLen == 0)
        return false;

    int same = 0;
    for (int i = 0; i < minLen; ++i)
        if (a[i] == b[i])
            same++;

    double ratio = same / double(minLen);
    return ratio >= 0.8;
}


// --- 3) Hamming-távolság (rövid tagokra) ---
inline bool hammingSimilar(const QString& a, const QString& b)
{
    if (a.size() != b.size())
        return false;

    if (a.size() > 3)
        return false;

    int diff = 0;
    for (int i = 0; i < a.size(); ++i)
        if (a[i] != b[i])
            diff++;

    return diff <= 1;
}


// --- 4) Általános fuzzy hasonlóság ---
inline bool tooSimilar(const QString& a, const QString& b)
{
    if (a == b)
        return false;

    if (levenshtein(a, b) <= 1)
        return true;

    if (hammingSimilar(a, b))
        return true;

    if (prefixSimilar(a, b))
        return true;

    return false;
}


// --- 5) Substring match ---
inline bool substringMatch(const QString& a, const QString& b)
{
    return a.contains(b) || b.contains(a);
}


// --- 6) Prefix match ---
inline bool prefixMatch(const QString& a, const QString& b)
{
    return a.startsWith(b) || b.startsWith(a);
}


// --- 7) Fuzzy match (MaterialSearchDialog-hoz) ---
inline bool fuzzyMatch(const QString& a, const QString& b)
{
    return tooSimilar(a, b) || substringMatch(a, b);
}


// --- 8) Ismétlődő rövid tagok detektálása ---
inline QStringList repeatedShortTags(const QStringList& tags)
{
    QHash<QString, int> counts;
    for (const QString& t : tags)
        counts[t]++;

    QStringList out;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.key().size() <= 2 && it.value() > 1)
            out.append(it.key());
    }
    return out;
}


// --- 9) Valószínű elgépelés detektálása ---
inline bool likelyTypo(const QString& a, const QString& b)
{
    return levenshtein(a, b) == 1;
}


// --- 10) Exact match ---
inline bool exactMatch(const QString& a, const QString& b)
{
    return a == b;
}


// --- 11) Multi-field fuzzy keresés (MaterialSearchDialog) ---
inline bool anyExact(const QStringList& fields, const QString& t)
{
    for (const auto& f : fields)
        if (exactMatch(f, t))
            return true;
    return false;
}

inline bool anyPrefix(const QStringList& fields, const QString& t)
{
    for (const auto& f : fields)
        if (prefixMatch(f, t))
            return true;
    return false;
}

inline bool anySubstring(const QStringList& fields, const QString& t)
{
    for (const auto& f : fields)
        if (substringMatch(f, t))
            return true;
    return false;
}

inline bool anyFuzzy(const QStringList& fields, const QString& t)
{
    for (const auto& f : fields)
        if (fuzzyMatch(f, t))
            return true;
    return false;
}

inline bool fuzzyDifferent(const QString& a, const QString& b)
{
    // Ha mindkettő rövid (pl. P1, P2, P3), akkor nem gyanús
    if (a.size() <= 3 && b.size() <= 3)
        return false;

    // Ha prefix teljesen eltér → gyanús
    if (!a.startsWith(b.left(1)) && !b.startsWith(a.left(1)))
        return true;

    // Ha Levenshtein túl nagy → gyanús
    if (levenshtein(a, b) >= 3)
        return true;

    return false;
}

} // namespace StringSimilarity
