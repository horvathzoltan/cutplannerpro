#include "cutplan_input_summary.h"
#include "common/texthelper.h"

#include <QMap>

QString CutPlanInputSummary::toText() const {
    QStringList out;

    out << "📥 input";
    //out << "────────────────────────────────";

    for (const auto& owner : owners) {
        out << QString("🏢 %1").arg(owner.ownerName);

        for (const auto& mb : owner.materials) {

            QString matLine = mb.materialBarcode.isEmpty()
            ? mb.materialName
            : QString("%1 (%2)").arg(mb.materialName, mb.materialBarcode);

            out << QString("Anyag: %1").arg(matLine);

            // aggregálás hossz szerint
            QMap<int, QStringList> lenToRefs;
            for (const auto& it : mb.items)
                lenToRefs[it.length_mm].append(it.externalRef);

            out << "🧾 Cut Request összefoglaló (aggregálva):";

            for (auto it = lenToRefs.constBegin(); it != lenToRefs.constEnd(); ++it) {

                QStringList refs = it.value();   // externalRef-ek pont nélkül

                // kompresszió
                QString compressed = TextHelper::compressRanges_String(refs);

                out << QString("  • %1 mm (%2 db) → %3")
                           .arg(it.key())
                           .arg(it.value().size())
                           .arg(compressed);
            }


            out << QString("  Összesen: %1 tételszám, összesen %2 darab")
                       .arg(mb.totalItems)
                       .arg(mb.totalQuantity);
        }
    }

    return out.join("\n");
}
