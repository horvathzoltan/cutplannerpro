#pragma once
#include <QRandomGenerator>
#include <QVector>
#include <QStringList>

#include "service/cutting/instruction/labelmodel.h"
#include "service/cutting/instruction/cuttinginstructionutils.h"
#include "emojihelper.h"

namespace TestLabelMaker {

inline QVector<LabelModel> makeCompactTestLabels()
{
    QVector<LabelModel> out;

    // 1) Prioritás ikonok (mindegyikből 5 db)
    QStringList prios = {
        EmojiHelper::priorityIconFor(-1),
        EmojiHelper::priorityIconFor(1),
        EmojiHelper::priorityIconFor(3),
        EmojiHelper::priorityIconFor(6),
        EmojiHelper::priorityIconFor(10)
    };

    // 2) Csoport ikonok (mindegyikből 1 db)
    QStringList groups = EmojiHelper::GROUP_ICONS;

    // 3) Owner nevek
    QStringList owners = {
        "Kovács Béla",
        "Nagy Alexandra",
        "Tóth László",
        "Extra Hosszú Tulajdonosnév Akinek A Neve Belelóg A Sorba",
        "XY"
    };

    // 4) ExternalRef-ek
    QStringList refs = { "1", "12", "123", "1234", "9999" };


    // 5) Méretek
    QStringList sizes = { "120 mm", "450 mm", "1230 mm", "9999 mm" };

    // 6) Extra mezők
    QStringList extras = {
        "",
        " (Bal)",
        " (Jobb)",
        " (Közép)",
        " (Speciális, Bal)"
    };

    QRandomGenerator* rng = QRandomGenerator::global();

    // ------------------------------------------------------------
    // A) PRIORITÁS TESZT – minden prio ikonból 5 címke
    // ------------------------------------------------------------
    for (const QString& pr : prios)
    {
        for (int i = 0; i < 5; ++i)
        {
            LabelModel lm;
            lm.priorityIcon = pr;
            lm.groupIcon = groups.at(rng->bounded(groups.size()));

            QString ref = refs.at(rng->bounded(refs.size()));
            lm.barcode = ref;

            lm.parts.append({ ref + ".", false, false, 0, Qt::AlignLeft });
            lm.parts.append({ owners.at(rng->bounded(owners.size())), true, true, 0, Qt::AlignCenter });

            QString ex = extras.at(rng->bounded(extras.size()));
            if (!ex.isEmpty())
                lm.parts.append({ ex, false, false, 0, Qt::AlignRight });

            lm.parts.append({ " | " + sizes.at(rng->bounded(sizes.size())), false, false, 0, Qt::AlignRight });

            out.append(lm);
        }
    }

    // ------------------------------------------------------------
    // B) CSOPORTIKON TESZT – minden group ikonból 1 címke
    // ------------------------------------------------------------
    for (const QString& gr : groups)
    {
        LabelModel lm;
        lm.priorityIcon = prios.at(rng->bounded(prios.size()));
        lm.groupIcon = gr;

        QString ref = refs.at(rng->bounded(refs.size()));
        lm.barcode = ref;

        lm.parts.append({ ref + ".", false, false, 0, Qt::AlignLeft });
        lm.parts.append({ owners.at(rng->bounded(owners.size())), true, true, 0, Qt::AlignCenter });


        QString ex = extras.at(rng->bounded(extras.size()));
        if (!ex.isEmpty())
            lm.parts.append({ ex, false, false, 0, Qt::AlignRight });

        lm.parts.append({ " | " + sizes.at(rng->bounded(sizes.size())), false, false, 0, Qt::AlignRight });

        out.append(lm);
    }

    // ------------------------------------------------------------
    // C) OWNER NAME TESZT – minden ownerből 2 címke
    // ------------------------------------------------------------
    for (const QString& own : owners)
    {
        for (int i = 0; i < 2; ++i)
        {
            LabelModel lm;
            lm.priorityIcon = prios.at(rng->bounded(prios.size()));
            lm.groupIcon = groups.at(rng->bounded(groups.size()));

            QString ref = refs.at(rng->bounded(refs.size()));
            lm.barcode = ref;

            lm.parts.append({ ref + ".", false, false, 0, Qt::AlignLeft });
            lm.parts.append({ own, true, true, 0, Qt::AlignCenter });

            QString ex = extras.at(rng->bounded(extras.size()));
            if (!ex.isEmpty())
                lm.parts.append({ ex, false, false, 0, Qt::AlignRight });

            lm.parts.append({ " | " + sizes.at(rng->bounded(sizes.size())), false, false, 0, Qt::AlignRight });

            out.append(lm);
        }
    }

    return out;
}

// ------------------------------------------------------------
// PDF generátor – main()‑ből hívható
// ------------------------------------------------------------
inline int runCompactPdfTest(const QString& outPath = "/home/zoli/test_labels_compact.pdf")
{
    QPdfWriter writer(outPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        qWarning("❌ Nem sikerült megnyitni a PDF fájlt.");
        return -1;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    QFont font("Noto Sans Mono", 11);
    painter.setFont(font);

    auto labels = makeCompactTestLabels();

    CuttingInstructionUtils::formatLabelColumnFlow_Pdf(
        labels,
        painter,
        writer,
        pageRect,
        2,
        240.0
        );

    painter.end();
    qInfo("🏷️ Kompakt teszt címkék PDF-je elkészült: %s", qUtf8Printable(outPath));
    return 0;
}

} // namespace TestLabelMaker
