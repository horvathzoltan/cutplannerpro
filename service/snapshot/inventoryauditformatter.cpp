#include "inventoryauditformatter.h"

#include <QStringList>

QString InventoryAuditFormatter::toText(
    const InventoryAuditModel& model)
{
    QStringList out;

    auto put =
        [&](QString& line,
           int pos,
           const QString& text)
    {
        for (int i = 0;
             i < text.length() &&
             pos + i < line.length();
             ++i)
        {
            line[pos + i] = text[i];
        }
    };

    const int colType     = 0;
    const int colBarcode  = 5;
    const int colDb       = 40;
    const int colMm       = 48;
    const int colGreedy   = 60;
    const int colStock    = 70;
    const int colSnap     = 80;

    {
        QString header(95, ' ');

        put(header, colType,    "T");
        put(header, colBarcode, "ANYAG");
        put(header, colDb,      "DB");
        put(header, colMm,      "MM");
        put(header, colGreedy,  "GREEDY");
        put(header, colStock,   "STOCK");
        put(header, colSnap,    "SNAP");

        out << header;
    }

    out << QString(95, '=');

    for (const auto& r : model.rows)
    {
        QString line(95, ' ');

        const QString marker =
            r.isExpanded ? "EXP" : "REQ";

        put(line, colType,    marker);
        put(line, colBarcode, r.barcode.left(32));
        put(line, colDb,      QString::number(r.requestPieces));
        put(line, colMm,      QString::number(r.requestedLengthMm));
        put(line, colGreedy,  QString::number(r.estimatedStrands));
        put(line, colStock,   QString::number(r.stockStrands));
        put(line, colSnap,    QString::number(r.snapshotStrands));

        out << line;

        if (!r.materialGroup.isEmpty())
        {
            out << QString("      group: %1")
                       .arg(r.materialGroup);
        }

        if (r.isExpanded)
        {
            out << QString("      expanded from: %1")
                       .arg(r.sourceBarcode);
        }

        if (!r.materialName.isEmpty())
        {
            out << QString("      %1")
                       .arg(r.materialName);
        }

        out << "";
    }

    out << "";
    out << QString(95, '=');
    out << "GROUP SUMMARY";
    out << QString(95, '=');
    out << "";

    for (const auto& g : model.groups)
    {
        out << QString("%1 (%2)")
        .arg(g.groupKey)
            .arg(g.groupName);

        out << QString("    original greedy : %1")
                   .arg(g.originalGreedyStrands);

        out << QString("    expanded greedy : %1")
                   .arg(g.expandedGreedyStrands);

        out << QString("    total greedy    : %1")
                   .arg(g.totalGreedyStrands);

        out << QString("    stock           : %1")
                   .arg(g.totalStockStrands);

        out << QString("    snapshot        : %1")
                   .arg(g.totalSnapshotStrands);

        out << QString("    status          : %1")
                   .arg(g.hasShortage ? "SHORTAGE" : "OK");

        out << "";
        out << QString(60, '-');
        out << "";
    }

    return out.join("\n");
}
