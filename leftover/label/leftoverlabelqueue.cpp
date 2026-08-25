#include "leftoverlabelqueue.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <common/filenamehelper.h>

//
// LabelModel → CSV sor
//
QString LeftoverLabelQueue::serialize(const LabelModel& lm) const
{
    QStringList partStrings;

    for (const auto& p : lm.parts)
    {
        QString ps = QString("%1^%2^%3^%4^%5^%6")
        .arg(p.text)
            .arg(p.targetRow)
            .arg(int(p.align))
            .arg(p.small)
            .arg(p.bold)
            .arg(p.italic);
        partStrings.append(ps);
    }

    return QString("%1;%2;%3;%4")
        .arg(lm.barcode)
        .arg(lm.priorityIcon)
        .arg(lm.groupIcon)
        .arg(partStrings.join("|"));
}

//
// CSV sor → LabelModel
//
LabelModel LeftoverLabelQueue::deserialize(const QString& line) const
{
    LabelModel lm;

    QStringList cols = line.split(';');
    if (cols.size() < 4)
        return lm;

    lm.barcode = cols[0];
    lm.priorityIcon = cols[1];
    lm.groupIcon = cols[2];

    QStringList partStrings = cols[3].split('|');
    for (const QString& ps : partStrings)
    {
        QStringList f = ps.split('^');
        if (f.size() < 6)
            continue;

        LabelPart p;
        p.text = f[0];
        p.targetRow = f[1].toInt();
        p.align = Qt::Alignment(f[2].toInt());
        p.small = f[3].toInt();
        p.bold = f[4].toInt();
        p.italic = f[5].toInt();

        lm.parts.append(p);
    }

    return lm;
}

//
// Append
//
void LeftoverLabelQueue::append(const LabelModel& lm)
{
    auto path = FileNameHelper::instance().getLeftoverLabelQueueCsvFile();
    QFile f(path);
    if (!f.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream ts(&f);
    ts << serialize(lm) << "\n";
}

//
// Load
//
QVector<LabelModel> LeftoverLabelQueue::load() const
{
    QVector<LabelModel> out;

    auto path = FileNameHelper::instance().getLeftoverLabelQueueCsvFile();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;

    QTextStream ts(&f);
    while (!ts.atEnd())
    {
        QString line = ts.readLine().trimmed();
        if (!line.isEmpty()) {
            LabelModel lm = deserialize(line);
            if (!lm.barcode.isEmpty())
                out.append(lm);
        }

    }

    return out;
}

//
// Clear
//
void LeftoverLabelQueue::clear()
{
    auto path = FileNameHelper::instance().getLeftoverLabelQueueCsvFile();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;

}
