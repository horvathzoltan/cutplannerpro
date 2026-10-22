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
    const int colAsm      = 80;
    const int colAvl      = 90;
    const int colSnap     = 100;
    const int colStatus   = 110;


    {
        QString header(105, ' ');

        put(header, colType,    "T");
        put(header, colBarcode, "ANYAG");
        put(header, colDb,      "DB");
        put(header, colMm,      "MM");
        put(header, colGreedy,  "GREEDY");
        put(header, colStock, "STOCK");
        put(header, colAsm,   "ASM");
        put(header, colAvl,   "AVL");
        put(header, colSnap,  "SNAP");

        out << header;
    }

    out << QString(60, '=');

    for (const auto& r : model.rows)
    {
        if (r.isExpanded)
            continue;

        QString line(105, ' ');

        const QString marker = "REQ";

        QString status;

        const int effectiveStock =
            r.availableStock;

        if (effectiveStock >= r.estimatedStrands)
        {
            status = "✅";
        }
        else if (effectiveStock > 0)
        {
            status = "⚠️";
        }
        else
        {
            status = "⭕";
        }


        put(line, colType,    marker);
        put(line, colBarcode, r.barcode.left(32));
        put(line, colDb,      QString::number(r.requestPieces));
        put(line, colMm,      QString::number(r.requestedLengthMm));
        put(line, colGreedy,  QString::number(r.estimatedStrands));
        put(line, colStock,   QString::number(r.stockStrands));
        put(line,
            colAsm,
            r.componentAssemblyStock > 0
                ? QString::number(
                      r.componentAssemblyStock)
                : "-");
        put(line, colAvl, QString::number(r.availableStock));
        put(line, colSnap,    QString::number(r.snapshotStrands));
        put(line, colStatus, status);

        out << line;

        if (!r.materialName.isEmpty())
        {
            out << QString("      %1")
                       .arg(r.materialName);
        }

        if (!r.assemblyComponents.isEmpty())
        {
            out << QString(
                       "      assembly capacity: %1")
                       .arg(
                           r.componentAssemblyStock);

            for (const auto& comp :
                 r.assemblyComponents)
            {
                out << QString(
                           "         + %1  stock:%2  need:%3  asm:%4")
                           .arg(comp.barcode)
                           .arg(comp.stock)
                           .arg(comp.requiredPerBundle)
                           .arg(comp.assemblableCount);
            }

            out << "";
        }

        for (const auto& child : model.rows)
        {
            if (!child.isExpanded)
                continue;

            if (child.originMaterialId != r.materialId)
                continue;

            QString childStatus =
                child.availableStock > 0
                    ? "🔁"
                    : "⚪";

            QString childLine(105, ' ');

            put(childLine, colType,    "->");
            put(childLine,
                colBarcode + 2,
                child.barcode.left(30));
            put(childLine, colGreedy,  QString::number(child.estimatedStrands));
            put(childLine, colStock,   QString::number(child.stockStrands));
            put(childLine,
                colAsm,
                child.componentAssemblyStock > 0
                    ? QString::number(
                          child.componentAssemblyStock)
                    : "-");
            put(childLine, colAvl, QString::number(child.availableStock));
            put(childLine, colSnap,    QString::number(child.snapshotStrands));
            put(childLine, colStatus, childStatus);

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
    out << QString(60, '=');
    out << "GROUP SUMMARY";
    out << QString(60, '=');
    out << "";

    for (const auto& g : model.groups)
    {
        out << QString("%1 (%2)")
        .arg(g.groupKey)
            .arg(g.groupName);

        out << QString("    required demand       : %1")
                   .arg(g.requiredDemand);

        out << QString("    original available    : %1")
                   .arg(g.originalAvailable);
        out << QString(
            "    source                : stock + assembly");

        out << QString("    alternative available : %1")
                   .arg(g.alternativeAvailable);

        out << QString("    total available       : %1")
                   .arg(g.totalAvailable);

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

        out << QString("    coverage              : %1")
                   .arg(coverageText);

        QString coverageExplanation;

        switch (g.coverage)
        {
        case GroupCoverage::Original:
            coverageExplanation =
                "original material availability satisfies demand";
            break;

        case GroupCoverage::Partial:
            coverageExplanation =
                "combined availability is required";
            break;

        case GroupCoverage::Substitute:
            coverageExplanation =
                "demand is satisfied by alternatives";
            break;

        case GroupCoverage::Shortage:
            coverageExplanation =
                "available inventory is insufficient";
            break;
        }

        out << QString("    explanation           : %1")
                   .arg(coverageExplanation);

        out << "";

        if (g.alternativeEstimate > 0)
        {
            out << "";
            out << "    coverage details";

            out << QString("        original estimate    : %1")
                       .arg(g.originalEstimate);

            out << QString("        alternative estimate : %1")
                       .arg(g.alternativeEstimate);
        }

        out << "";
        out << QString(60, '-');
        out << "";
    }

    return out.join("\n");
}
