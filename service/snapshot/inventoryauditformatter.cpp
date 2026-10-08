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
    const int colStatus   = 90;

    {
        QString header(95, ' ');

        put(header, colType,    "T");
        put(header, colBarcode, "ANYAG");
        put(header, colDb,      "DB");
        put(header, colMm,      "MM");
        put(header, colGreedy,  "GREEDY");
        put(header, colStock,   "STOCK");
        put(header, colSnap,    "SNAP");
        put(header, colStatus,  "OK");

        out << header;
    }

    out << QString(95, '=');

    for (const auto& r : model.rows)
    {
        if (r.isExpanded)
            continue;

        QString line(95, ' ');

        const QString marker =
            r.isExpanded ? "EXP" : "REQ";

        QString status;

        if (r.isExpanded)
        {
            status = "🔄";
        }
        else
        {
            status =
                r.snapshotStrands >= r.estimatedStrands
                    ? "✅"
                    : "❌";
        }
        put(line, colType,    marker);
        put(line, colBarcode, r.barcode.left(32));
        put(line, colDb,      QString::number(r.requestPieces));
        put(line, colMm,      QString::number(r.requestedLengthMm));
        put(line, colGreedy,  QString::number(r.estimatedStrands));
        put(line, colStock,   QString::number(r.stockStrands));
        put(line, colSnap,    QString::number(r.snapshotStrands));
        put(line, colStatus, status);

        out << line;



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

        for (const auto& child : model.rows)
        {
            if (!child.isExpanded)
                continue;

            if (child.originMaterialId != r.materialId)
                continue;

            QString childLine(95, ' ');

            put(childLine, colType,    "->");
            put(childLine, colBarcode, child.barcode.left(32));
            put(childLine, colGreedy,  QString::number(child.estimatedStrands));
            put(childLine, colStock,   QString::number(child.stockStrands));
            put(childLine, colSnap,    QString::number(child.snapshotStrands));
            put(childLine, colStatus,  "ALT");

            out << childLine;

            if (!child.materialName.isEmpty())
            {
                out << QString("         %1")
                .arg(child.materialName);
            }

            if (!child.sourceBarcode.isEmpty())
            {
                out << QString("         origin: %1")
                .arg(child.sourceBarcode);
            }
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

        out << QString("    request demand  : %1")
                   .arg(g.originalGreedyStrands);

        out << QString("    expanded demand : %1")
                   .arg(g.expandedGreedyStrands);

        out << QString("    total demand    : %1")
                   .arg(g.totalGreedyStrands);

        out << QString("    stock           : %1")
                   .arg(g.totalStockStrands);

        out << QString("    snapshot        : %1")
                   .arg(g.totalSnapshotStrands);

        QString coverageText;

        switch (g.coverage)
        {
        case GroupCoverage::Original:
            coverageText = "✅ ORIGINAL";
            break;

        case GroupCoverage::Partial:
            coverageText = "🔄 PARTIAL";
            break;

        case GroupCoverage::Substitute:
            coverageText = "🔁 SUBSTITUTE";
            break;

        case GroupCoverage::Shortage:
            coverageText = "❌ SHORTAGE";
            break;
        }

        out << QString("    coverage        : %1")
                   .arg(coverageText);

        out << "";
        out << QString(60, '-');
        out << "";
    }

    return out.join("\n");
}
