#pragma once

#include <QSet>
#include <QStringList>


namespace TextHelper {


static QString compressRanges_int(QList<int> nums)
{
    if (nums.isEmpty())
        return "—";

    // ismétlődések kiszűrése
    QSet<int> uniq;
    for (int n : nums)
        uniq.insert(n);

    nums = uniq.values();
    std::sort(nums.begin(), nums.end());


    QStringList out;
    int start = nums.first();
    int prev  = start;

    for (int i = 1; i < nums.size(); ++i) {
        int n = nums[i];
        if (n == prev + 1) {
            prev = n;
            continue;
        }

        // lezárunk egy tartományt
        if (start == prev)
            out << QString::number(start) + ".";
        else
            out << QString("%1–%2.").arg(start).arg(prev);

        start = prev = n;
    }

    // utolsó tartomány lezárása
    if (start == prev)
        out << QString::number(start) + ".";
    else
        out << QString("%1–%2.").arg(start).arg(prev);

    return out.join(", ");
}


static QString compressRanges_String(const QStringList& refs)
{
    if (refs.isEmpty())
        return "—";

    QList<int> nums;
    for (const QString& r : refs){
        // csak az első számot vesszük ki
        QString cleaned = r.section(' ', 0, 0);   // "37 1/2" → "37"

        bool ok = false;
        int n =  cleaned.toInt(&ok);
        if(ok)
            nums.append(n);
    }

    auto a = compressRanges_int(nums);
    return a;
}


static QStringList wrapSeparated(const QString& text,
                                 const QString& sep = ",",
                                 int width = 80)
{
    // 1) tokenizálás szeparátor alapján
    QStringList tokens = text.split(sep, Qt::SkipEmptyParts);

    QStringList lines;
    QString current;

    for (QString tok : tokens) {

        tok = tok.trimmed();   // fontos!

        QString candidate =
            current.isEmpty()
                ? tok
                : current + sep + " " + tok;

        if (candidate.length() > width) {
            if (!current.isEmpty())
                lines << current;
            current = tok;
        } else {
            current = candidate;
        }
    }

    if (!current.isEmpty())
        lines << current;

    return lines;
}

static QStringList smartJoin(const QStringList& tokens,
                             const QString& sep = ", ",
                             int width = 80)
{
    QStringList lines;
    QString current;

    for (const QString& tok : tokens) {

        QString candidate =
            current.isEmpty()
                ? tok
                : current + sep + tok;

        if (candidate.length() > width) {
            if (!current.isEmpty())
                lines << current;
            current = tok;
        } else {
            current = candidate;
        }
    }

    if (!current.isEmpty())
        lines << current;

    return lines;
}



} // end namespace TextHelper
