#include "cutplansummary.h"
#include <QRegularExpression>
#include <QStringList>


static QStringList wrapList(const QStringList& items, int maxWidth, const QString& indent)
{
    QStringList lines;
    QString current = indent;

    for (const QString& it : items) {
        QString token = it + ", ";

        if (current.length() + token.length() > maxWidth) {
            lines << current.trimmed();
            current = indent + token;
        } else {
            current += token;
        }
    }

    if (!current.trimmed().isEmpty())
        lines << current.trimmed().remove(QRegularExpression(",\\s*$"));

    return lines;
}

QString CutPlanSummary::toText() const
{
    QStringList out;

    //
    // 📄 Fejléc
    //
    out << "📄 Cut Plan Summary";
    out << "────────────────────────────────";



    //
    // 📊 Globális összefoglaló
    //
    out << "📊 Anyagfelhasználás (összesen):";
    out << QString("  • Szálak száma: %1 db").arg(rodCount);
    out << QString("  • Szálak teljes hossza: %1 m")
               .arg(QString::number(rodTotal_m, 'f', 2));

    out << QString("  • Levágott darabok: %1 db (%2 m)")
               .arg(pieceCount)
               .arg(QString::number(pieceTotal_m, 'f', 2));

    out << QString("  • Kerf: %1 db (%2 mm)")
               .arg(kerfCount)
               .arg(kerfTotal_mm);

    out << QString("  • Hulladék: %1 db (%2 mm)")
               .arg(wasteCount)
               .arg(wasteTotal_mm);

    out << QString("  • Szakaszok összesen: %1 db")
               .arg(segmentCount);



    //
    // 📦 Anyagonkénti összefoglaló
    //
    out << "────────────────────────────────";
    out << "📦 Anyagonkénti összefoglaló:";

    for (const auto& m : materials) {

        QString group = m.materialGroupName.isEmpty() ? "nincs" : m.materialGroupName;
        out << QString("  • Anyag: %1 csoport:(%2)").arg(m.materialName, group);


        out << QString("    - Szálak: %1 db (%2 m)")
                   .arg(m.rodCount)
                   .arg(QString::number(m.rodTotal_m, 'f', 2));

        out << QString("    - Darabok: %1 db (%2 m)")
                   .arg(m.pieceCount)
                   .arg(QString::number(m.pieceTotal_m, 'f', 2));

        //
        // 🧾 Tételszámok listázása
        //
        if (m.itemRefs.isEmpty()) {
            out << "    - Tételszámok: —";
        } else {
            QStringList refs = QStringList(m.itemRefs.begin(), m.itemRefs.end());
            std::sort(refs.begin(), refs.end());

            out << "    - Tételszámok:";

            QStringList wrapped = wrapList(refs, 80, "        "); // 8 space indent
            for (const auto& line : wrapped)
                out << line;
        }

    }



    //
    // ♻️ Leftover statisztika
    //
    out << "────────────────────────────────";
    out << "♻️ Maradékok:";
    out << QString("  • Újrahasználható leftover: %1 db").arg(reusableCount);
    out << QString("  • Archivált végmaradék: %1 db").arg(archivedCount);



    //
    // 🚦 Hatékonyság
    //
    out << "────────────────────────────────";
    out << "🚦 Hatékonyság:";
    out << QString("  • Anyagfelhasználási mutató: %1 %")
               .arg(QString::number(efficiency, 'f', 1));



    return out.join("\n");
}


