#include "cutplan_output_summary.h"
#include "common/texthelper.h"

QString CutPlanOutputSummary::toText() const {
    QStringList out;

    out << "📤 output";
    out << "📊 Anyagfelhasználás (összesen):";

    out << QString("  • Szálak száma: %1 db").arg(rodCount);
    out << QString("  • Szálak teljes hossza: %1 m").arg(rodTotal_m, 0, 'f', 2);
    out << QString("  • Levágott darabok: %1 db (%2 m)").arg(pieceCount).arg(pieceTotal_m, 0, 'f', 2);
    out << QString("  • Kerf: %1 db (%2 mm)").arg(kerfCount).arg(kerfTotal_mm);
    out << QString("  • Hulladék: %1 db (%2 mm)").arg(wasteCount).arg(wasteTotal_mm);
    out << QString("  • Szakaszok összesen: %1 db").arg(segmentCount);

    out << "────────────────────────────────";
    out << "📦 Anyagonkénti összefoglaló:";

    for (const auto& m : materials) {
        QString group = m.materialGroupName.isEmpty() ? "nincs" : m.materialGroupName;

        out << QString("  • Anyag: %1 csoport:(%2)").arg(m.materialName, group);
        out << QString("    - Szálak: %1 db (%2 m)").arg(m.rodCount).arg(m.rodTotal_m, 0, 'f', 2);
        out << QString("    - Darabok: %1 db (%2 m)").arg(m.pieceCount).arg(m.pieceTotal_m, 0, 'f', 2);

        if (m.itemRefs.isEmpty()) {
            out << "    - Tételszámok: —";
        } else {
            QStringList refs = QStringList(m.itemRefs.begin(), m.itemRefs.end());
            std::sort(refs.begin(), refs.end());

            QString compressed = TextHelper::compressRanges_String(refs);

            out << QString("    - Tételszámok: %1").arg(compressed);
        }
    }

    out << "────────────────────────────────";
    out << "♻️ Maradékok:";
    out << QString("  • Újrahasználható leftover: %1 db").arg(reusableCount);
    out << QString("  • Archivált végmaradék: %1 db").arg(archivedCount);

    out << "────────────────────────────────";
    out << "🚦 Hatékonyság:";
    out << QString("  • Anyagfelhasználási mutató: %1 %").arg(efficiency, 0, 'f', 1);

    return out.join("\n");
}

