// #include "cutplansummary.h"
// #include <QRegularExpression>
// #include <QStringList>
// #include <materials/model/material_master.h>
// #include <materials/registry/material_registry.h>


// static QStringList wrapList(const QStringList& items, int maxWidth, const QString& indent)
// {
//     QStringList lines;
//     QString current = indent;

//     for (const QString& it : items) {
//         QString token = it + ", ";

//         if (current.length() + token.length() > maxWidth) {
//             lines << current.trimmed();
//             current = indent + token;
//         } else {
//             current += token;
//         }
//     }

//     if (!current.trimmed().isEmpty())
//         lines << current.trimmed().remove(QRegularExpression(",\\s*$"));

//     return lines;
// }

// QString CutPlanSummary::toText() const
// {
//     QStringList out;

//     //
//     // 📄 Fejléc
//     //
//     out << "📄 Cut Plan Summary";
//     out << "────────────────────────────────";


//     // ÚJ: Gyártási input összefoglaló
//     out << "📥 Gyártási input összefoglaló";
//     out << "────────────────────────────────";

//     for (const auto& rs : inputSummaries) {
//         out << QString("🏢 %1").arg(rs.ownerName);

//         for (const auto& mb : rs.materials) {

//             QString matLine = mb.materialBarcode.isEmpty()
//             ? mb.materialName
//             : QString("%1 (%2)").arg(mb.materialName, mb.materialBarcode);

//             out << QString("Anyag: %1").arg(matLine);

//             // Hossz → tételszámok aggregálása
//             QMap<int, QStringList> lenToRefs;

//             for (const auto& it : mb.items) {
//                 lenToRefs[it.length_mm].append(it.externalRef);
//             }

//             out << "🧾 Cut Request összefoglaló (aggregálva):";

//             for (auto it = lenToRefs.constBegin(); it != lenToRefs.constEnd(); ++it) {
//                 int len = it.key();
//                 const QStringList& refs = it.value();

//                 QStringList refStrs;
//                 for (const QString& r : refs)
//                     refStrs << QString("%1.").arg(r);

//                 out << QString("  • %1 mm (%2 db) → %3")
//                            .arg(len)
//                            .arg(refs.size())
//                            .arg(refStrs.join(", "));
//             }

//             out << QString("  Összesen: %1 tételszám, összesen %2 darab")
//                        .arg(rs.totalItems)
//                        .arg(rs.totalQuantity);
//         }
//     }


//     out << "────────────────────────────────";
//     out << "🛠️ Gyártási előkészítés";
//     out << QString("  • END_TRIM_MM (eleje): %1 mm").arg(endTrim_mm);
//     out << QString("  • END_TRIM_MM (vége): %1 mm").arg(endTrim_mm);
//     out << QString("  • MINIMUM_HULLO_MM: %1 mm").arg(minimumHull_mm);
//     out << "────────────────────────────────";
//     out << "🧾 Cut Request összefoglaló (aggregálva):";
//     out << "📊 Anyagfelhasználás (összesen):";

//     out << QString("  • Szálak száma: %1 db").arg(rodCount);
//     out << QString("  • Szálak teljes hossza: %1 m")
//                .arg(QString::number(rodTotal_m, 'f', 2));

//     out << QString("  • Levágott darabok: %1 db (%2 m)")
//                .arg(pieceCount)
//                .arg(QString::number(pieceTotal_m, 'f', 2));

//     out << QString("  • Kerf: %1 db (%2 mm)")
//                .arg(kerfCount)
//                .arg(kerfTotal_mm);

//     out << QString("  • Hulladék: %1 db (%2 mm)")
//                .arg(wasteCount)
//                .arg(wasteTotal_mm);

//     out << QString("  • Szakaszok összesen: %1 db")
//                .arg(segmentCount);

//     //
//     // 📦 Anyagonkénti összefoglaló
//     //
//     out << "────────────────────────────────";
//     out << "📦 Anyagonkénti összefoglaló:";

//     for (const auto& m : materials) {

//         QString group = m.materialGroupName.isEmpty() ? "nincs" : m.materialGroupName;
//         out << QString("  • Anyag: %1 csoport:(%2)").arg(m.materialName, group);


//         out << QString("    - Szálak: %1 db (%2 m)")
//                    .arg(m.rodCount)
//                    .arg(QString::number(m.rodTotal_m, 'f', 2));

//         out << QString("    - Darabok: %1 db (%2 m)")
//                    .arg(m.pieceCount)
//                    .arg(QString::number(m.pieceTotal_m, 'f', 2));

//         //
//         // 🧾 Tételszámok listázása
//         //
//         if (m.itemRefs.isEmpty()) {
//             out << "    - Tételszámok: —";
//         } else {
//             QStringList refs = QStringList(m.itemRefs.begin(), m.itemRefs.end());
//             std::sort(refs.begin(), refs.end());

//             out << "    - Tételszámok:";

//             QStringList wrapped = wrapList(refs, 80, "        "); // 8 space indent
//             for (const auto& line : wrapped)
//                 out << line;
//         }

//     }



//     //
//     // ♻️ Leftover statisztika
//     //
//     out << "────────────────────────────────";
//     out << "♻️ Maradékok:";
//     out << QString("  • Újrahasználható leftover: %1 db").arg(reusableCount);
//     out << QString("  • Archivált végmaradék: %1 db").arg(archivedCount);



//     //
//     // 🚦 Hatékonyság
//     //
//     out << "────────────────────────────────";
//     out << "🚦 Hatékonyság:";
//     out << QString("  • Anyagfelhasználási mutató: %1 %")
//                .arg(QString::number(efficiency, 'f', 1));



//     return out.join("\n");
// }


