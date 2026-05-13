#pragma once

#include <QStringList>


namespace TextHelper {


static QString compressRanges_int(QList<int>& nums)
{
    if (nums.isEmpty())
        return "—";

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
        bool ok = false;
        int n =  r.toInt(&ok);
        if(ok) nums.append(n);
    }

    auto a = compressRanges_int(nums);
    return a;
}
} // end namespace TextHelper
