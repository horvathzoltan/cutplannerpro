#pragma once

#include <QPainter>
#include <QMap>

#include <stock/model/stockentry.h>
#include <materials/registry/material_registry.h>
#include <materialbundles/registry/bundle_registry.h>
#include <storage/registry/storageregistry.h>

namespace StockListFormUtils {

inline void drawStockListTable(
    QPainter& painter,
    const QRectF& pageRect,
    const QList<StockEntry>& entries
    ){


    QFontMetrics fm(painter.font());
    qreal lineH = fm.height() * 2;      // leftover-stílusú cellamagasság
    const qreal topMargin = 40.0;
    const qreal leftMargin = 40.0;
    const qreal gap = 8.0;


    qreal y = pageRect.top();// + topMargin;

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(Qt::black, 0.75));

    // === 1) Fejléc ===
    painter.drawText(QRectF(leftMargin, y, pageRect.width(), fm.height()),
                     Qt::AlignLeft,
                     "📦 Készletlista");
    y += fm.height();

    painter.drawText(QRectF(leftMargin, y, pageRect.width(), fm.height()),
                     Qt::AlignLeft,
                     QString("📅 Dátum: %1")
                         .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm")));
    y += fm.height() + gap;

    // === 2) Aggregálás per anyag ===
    QMap<QUuid, int> aggregated;
    for (const auto& e : entries) {
        aggregated[e.materialId] += e.quantity;
    }

    // === 3) Oszlopszélességek ===
    qreal totalW = pageRect.width() - leftMargin * 2;

    qreal col1 = totalW * 0.11; // External Code
    qreal col2 = totalW * 0.75; // Material Name
    qreal col3 = totalW * 0.07; // Qty
    qreal col4 = totalW * 0.07; // Broken (csak komponenseknél)

    qreal x1 = leftMargin;
    qreal x2 = leftMargin + col1;
    qreal x3 = leftMargin + col1 + col2;
    qreal x4 = leftMargin + col1 + col2 + col3;

    auto drawRow = [&](qreal yRow,
                       const QString& t1,
                       const QString& t2,
                       const QString& t3,
                       const QString& t4)
    {
        painter.drawText(QRectF(x1 + 5, yRow, col1 - 10, lineH), Qt::AlignLeft, t1);
        painter.drawText(QRectF(x2 + 5, yRow, col2 - 10, lineH), Qt::AlignLeft, t2);
        painter.drawText(QRectF(x3 + 5, yRow, col3 - 10, lineH), Qt::AlignLeft, t3);
        painter.drawText(QRectF(x4 + 5, yRow, col4 - 10, lineH), Qt::AlignLeft, t4);
    };

    auto drawFrame = [&](qreal top, qreal bottom, bool header = false)
    {
        if (header)
            painter.drawLine(leftMargin, top, leftMargin + totalW, top);

        painter.drawLine(leftMargin, bottom, leftMargin + totalW, bottom);

        painter.drawLine(x1, top, x1, bottom);
        painter.drawLine(x2, top, x2, bottom);
        painter.drawLine(x3, top, x3, bottom);
        painter.drawLine(x4, top, x4, bottom);
    };

    // === 4) Két soros leftover-stílusú táblázatfejléc ===
    qreal headerTop = y;

    drawRow(y, "External", "Material", "Qty", "Qty");
    y += lineH/2;

    drawRow(y, "code", "name", "pkg", "pcs");
    y += lineH/2;

    qreal headerBottom = y;
    drawFrame(headerTop, headerBottom, true);

    // 🔥 Checkbox rajzoló lambda
    // 🔥 Checkbox rajzoló lambda – több soros, 20 / sor
    auto drawCheckboxesForMaterial = [&](qreal& xPos, qreal& yPos,
                                         int totalPieces)
    {
        if (totalPieces <= 0)
            return;

        qreal checkboxLineH = lineH /2;
        const int maxPerLine = 25;

        int drawn = 0;

        while (drawn < totalPieces) {

            QString checkboxLine;
            int counter = 0;

            int remaining = totalPieces - drawn;
            int thisLineCount = qMin(remaining, maxPerLine);

            for (int i = 0; i < thisLineCount; ++i) {
                checkboxLine += "□ ";
                counter++;
                drawn++;

                if (counter == 5 && i != thisLineCount - 1) {
                    checkboxLine += "| ";
                    counter = 0;
                }
            }

            painter.drawText(
                QRectF(xPos, yPos, pageRect.width(), checkboxLineH),
                Qt::AlignLeft,
                checkboxLine
                );

            yPos += checkboxLineH;
        }
    };

    const qreal indent = 25.0;         // 🔥 egy tabbal bentebb

    qreal xu = x2+indent;

    // === 5) Sorok (aggregált készlet + bundle részletezés) ===
    for (auto it = aggregated.begin(); it != aggregated.end(); ++it) {

        QUuid materialId = it.key();
        int qty = it.value();

        const MaterialMaster* master = MaterialRegistry::instance().findById(materialId);
        QString ext = master ? master->externalCode : "";
        if (ext.length() > 12)
            ext = ext.left(12) + "…";
        QString name = master ? master->toReportLabel() : "";

        qreal blockTop = y;   // 🔥 blokk teteje (anyag + részletek)

        // --- FŐ SOR ---
        if (master && master->kind == MaterialKind::Bundle) {
            // 🔥 bundle → Qty = csomagok száma, Broken = üres
            drawRow(y, ext, name, QString::number(qty), "");
        } else {
            // 🔥 simple → Qty = üres, Broken = darabszám
            drawRow(y, ext, name, "", QString::number(qty));
        }

        y += lineH/2;

        // --- BUNDLE RÉSZLETEZÉS ---
        if (master && master->kind == MaterialKind::Bundle) {

            const BundleDefinition* def =
                BundleRegistry::instance().findByCode(master->bundleCode);

            if (def) {

                // 🔥 kisebb, dőlt font a részletező sorokhoz
                QFont smallFont = painter.font();
                smallFont.setPointSizeF(smallFont.pointSizeF() * 0.85);
                smallFont.setItalic(true);

                qreal detailLineH = lineH * 0.8;   // 🔥 kisebb cellamagasság

                for (const auto& comp : def->components) {

                    const MaterialMaster* cm =
                        MaterialRegistry::instance().findById(comp.materialId);

                    QString cext = cm ? cm->externalCode : "";

                    if (cext.length() > 12)
                        cext = cext.left(12) + "…";

                    QString cname = cm ? cm->toReportLabel() : "";

                    int compTotal = qty * comp.count;   // 🔥 darabszám

                    painter.save();
                    painter.setFont(smallFont);

                    // 🔥 részletező sor indentálva
                    painter.drawText(
                        QRectF(x1 + indent, y, col1 - indent, detailLineH),
                        Qt::AlignLeft,
                        cext
                        );
                    painter.drawText(
                        QRectF(x2 + indent, y, col2 - indent, detailLineH),
                        Qt::AlignLeft,
                        cname
                        );

                    // 🔥 Broken oszlop → darabszám
                    painter.drawText(
                        QRectF(x4 + 5, y, col4 - 10, detailLineH),
                        Qt::AlignLeft,
                        QString::number(compTotal)
                        );

                    painter.restore();

                    y += detailLineH/2;

                    // 🔥 checkbox sor a bundle komponensek után

                    drawCheckboxesForMaterial(xu, y, compTotal);
                }
            }
        }

        if (!master || master->kind != MaterialKind::Bundle) {
            // simple anyag → darabszám = qty
            drawCheckboxesForMaterial(xu, y, qty);
        }

        qreal blockBottom = y;   // 🔥 blokk alja

        // 🔥 KERET az anyag + részletező sorok köré
        drawFrame(blockTop, blockBottom);

        // 🔥 Következő anyag előtt elválasztó vonal
        painter.drawLine(leftMargin, blockBottom, leftMargin + totalW, blockBottom);

        // oldaltörés
        if (y + lineH > pageRect.bottom()) {
            painter.drawLine(leftMargin, y, leftMargin + totalW, y);
            return;
        }
    }

}

inline void drawStockListTable_2(
    QPainter& painter,
    const QRectF& pageRect,
    const QList<StockEntry>& entries
    ){


    QFontMetrics fm(painter.font());
    qreal lineH = fm.height() * 2;      // leftover-stílusú cellamagasság
    const qreal topMargin = 40.0;
    const qreal leftMargin = 40.0;
    const qreal gap = 8.0;


    qreal y = pageRect.top();// + topMargin;

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(Qt::black, 0.75));

    // === 1) Fejléc ===
    painter.drawText(QRectF(leftMargin, y, pageRect.width(), fm.height()),
                     Qt::AlignLeft,
                     "📦 Készletlista");
    y += fm.height();

    painter.drawText(QRectF(leftMargin, y, pageRect.width(), fm.height()),
                     Qt::AlignLeft,
                     QString("📅 Dátum: %1")
                         .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm")));
    y += fm.height() + gap;

    // === 2) Aggregálás per anyag ===
    QMap<QUuid, int> aggregated;
    for (const auto& e : entries) {
        aggregated[e.materialId] += e.quantity;
    }

    // === 3) Oszlopszélességek ===
    qreal totalW = pageRect.width() - leftMargin * 2;

    qreal col1 = totalW * 0.11; // External Code
    qreal col2 = totalW * 0.75; // Material Name
    qreal col3 = totalW * 0.07; // Qty
    qreal col4 = totalW * 0.07; // Broken (csak komponenseknél)

    qreal x1 = leftMargin;
    qreal x2 = leftMargin + col1;
    qreal x3 = leftMargin + col1 + col2;
    qreal x4 = leftMargin + col1 + col2 + col3;

    auto drawRow = [&](qreal yRow,
                       const QString& t1,
                       const QString& t2,
                       const QString& t3,
                       const QString& t4)
    {
        painter.drawText(QRectF(x1 + 5, yRow, col1 - 10, lineH), Qt::AlignLeft, t1);
        painter.drawText(QRectF(x2 + 5, yRow, col2 - 10, lineH), Qt::AlignLeft, t2);
        painter.drawText(QRectF(x3 + 5, yRow, col3 - 10, lineH), Qt::AlignLeft, t3);
        painter.drawText(QRectF(x4 + 5, yRow, col4 - 10, lineH), Qt::AlignLeft, t4);
    };

    auto drawFrame = [&](qreal top, qreal bottom, bool header = false)
    {
        if (header)
            painter.drawLine(leftMargin, top, leftMargin + totalW, top);

        painter.drawLine(leftMargin, bottom, leftMargin + totalW, bottom);

        painter.drawLine(x1, top, x1, bottom);
        painter.drawLine(x2, top, x2, bottom);
        painter.drawLine(x3, top, x3, bottom);
        painter.drawLine(x4, top, x4, bottom);
    };

    // === 4) Két soros leftover-stílusú táblázatfejléc ===
    qreal headerTop = y;

    drawRow(y, "External", "Material", "Qty", "Qty");
    y += lineH/2;

    drawRow(y, "code", "name", "pkg", "pcs");
    y += lineH/2;

    qreal headerBottom = y;
    drawFrame(headerTop, headerBottom, true);

    // 🔥 Checkbox rajzoló lambda
    // 🔥 Checkbox rajzoló lambda – több soros, 20 / sor
    auto drawCheckboxesForMaterial = [&](qreal& xPos, qreal& yPos,
                                         int totalPieces)
    {
        if (totalPieces <= 0)
            return;

        qreal checkboxLineH = lineH /2;
        const int maxPerLine = 25;

        int drawn = 0;

        while (drawn < totalPieces) {

            QString checkboxLine;
            int counter = 0;

            int remaining = totalPieces - drawn;
            int thisLineCount = qMin(remaining, maxPerLine);

            for (int i = 0; i < thisLineCount; ++i) {
                checkboxLine += "□ ";
                counter++;
                drawn++;

                if (counter == 5 && i != thisLineCount - 1) {
                    checkboxLine += "| ";
                    counter = 0;
                }
            }

            painter.drawText(
                QRectF(xPos, yPos, pageRect.width(), checkboxLineH),
                Qt::AlignLeft,
                checkboxLine
                );

            yPos += checkboxLineH;
        }
    };

    const qreal indent = 25.0;         // 🔥 egy tabbal bentebb

    qreal xu = x2+indent;

    // === 5) Sorok (aggregált készlet + bundle részletezés) ===
    for (auto it = aggregated.begin(); it != aggregated.end(); ++it) {

        QUuid materialId = it.key();
        int qty = it.value();

        const MaterialMaster* master = MaterialRegistry::instance().findById(materialId);
        QString ext = master ? master->externalCode : "";
        if (ext.length() > 12)
            ext = ext.left(12) + "…";
        QString name = master ? master->toReportLabel() : "";

        qreal blockTop = y;   // 🔥 blokk teteje (anyag + részletek)

        // --- FŐ SOR ---
        if (master && master->kind == MaterialKind::Bundle) {
            // 🔥 bundle → Qty = csomagok száma, Broken = üres
            drawRow(y, ext, name, QString::number(qty), "");
        } else {
            // 🔥 simple → Qty = üres, Broken = darabszám
            drawRow(y, ext, name, "", QString::number(qty));
        }

        y += lineH/2;

        // --- BUNDLE RÉSZLETEZÉS ---
        if (master && master->kind == MaterialKind::Bundle) {

            const BundleDefinition* def =
                BundleRegistry::instance().findByCode(master->bundleCode);

            if (def) {

                // 🔥 kisebb, dőlt font a részletező sorokhoz
                QFont smallFont = painter.font();
                smallFont.setPointSizeF(smallFont.pointSizeF() * 0.85);
                smallFont.setItalic(true);

                qreal detailLineH = lineH * 0.8;   // 🔥 kisebb cellamagasság

                for (const auto& comp : def->components) {

                    const MaterialMaster* cm =
                        MaterialRegistry::instance().findById(comp.materialId);

                    QString cext = cm ? cm->externalCode : "";

                    if (cext.length() > 12)
                        cext = cext.left(12) + "…";

                    QString cname = cm ? cm->toReportLabel() : "";

                    int compTotal = qty * comp.count;   // 🔥 darabszám

                    painter.save();
                    painter.setFont(smallFont);

                    // 🔥 részletező sor indentálva
                    painter.drawText(
                        QRectF(x1 + indent, y, col1 - indent, detailLineH),
                        Qt::AlignLeft,
                        cext
                        );
                    painter.drawText(
                        QRectF(x2 + indent, y, col2 - indent, detailLineH),
                        Qt::AlignLeft,
                        cname
                        );

                    // 🔥 Broken oszlop → darabszám
                    painter.drawText(
                        QRectF(x4 + 5, y, col4 - 10, detailLineH),
                        Qt::AlignLeft,
                        QString::number(compTotal)
                        );

                    painter.restore();

                    y += detailLineH/2;

                    // 🔥 checkbox sor a bundle komponensek után

                    drawCheckboxesForMaterial(xu, y, compTotal);
                }
            }
        }

        if (!master || master->kind != MaterialKind::Bundle) {
            // simple anyag → darabszám = qty
            drawCheckboxesForMaterial(xu, y, qty);
        }

        qreal blockBottom = y;   // 🔥 blokk alja

        // 🔥 KERET az anyag + részletező sorok köré
        drawFrame(blockTop, blockBottom);

        // 🔥 Következő anyag előtt elválasztó vonal
        painter.drawLine(leftMargin, blockBottom, leftMargin + totalW, blockBottom);

        // oldaltörés
        if (y + lineH > pageRect.bottom()) {
            painter.drawLine(leftMargin, y, leftMargin + totalW, y);
            return;
        }
    }

}

struct AggregatedSte{
    //QUuid storageId;
    //int quantity;

    int componentCount;           // pack darabszám
    int packQuantity;

    QString packBarcode;     // csomagolási egység vonalkód

    QDateTime lastSeenAt;

    QString logisticBarcode;
    QString humanName;
};

inline AggregatedSte buildAggregatedSte(const StockEntry& e,
                                        int componentCount)
{
    AggregatedSte a;

    //a.storageId = e.storageId;
    a.lastSeenAt = e.lastSeenAt;

    // pack info
    a.packBarcode = e.materialBarcode();
    a.componentCount = componentCount;
    a.packQuantity = e.quantity;

    // storage info
    a.logisticBarcode = StorageRegistry::instance().logisticBarcode(e.storageId);
    a.humanName = StorageRegistry::instance().uniqueHumanName(e.storageId);

    // opcionális bővítések:
    // a.storagePath = StorageUtils::buildPathTree(e.storageId);
    // a.storageGroup = StorageRegistry::instance().groupName(e.storageId);

    return a;
}



struct AggregatedMaterial {
    QUuid materialId;
    //const MaterialMaster* master;
    int totalQty;
    QList<AggregatedSte> storages;   // hol és mennyi volt

    QString sectionKey;   // ← ÚJ: szekció-határ kulcs
};

struct RowResult {
    qreal newY;             // meddig jutott a rajzolás
    int nextStorageIndex;   // storage sorok közül hol kell folytatni
    bool pageBreakNeeded;   // kellett-e oldaltörés
};

inline RowResult drawSingleStockRow(
    QPainter& painter,
    const QRectF& pageRect,
    qreal y,
    const AggregatedMaterial& aggregatedMaterial,
    int startStorageIndex)
{
    const qreal leftMargin = 40.0;
    QFontMetrics fm(painter.font());
    qreal lineH = fm.height()*1.2;

    // --- FŐ SOR ---
    qreal needed = lineH;
    if (y + needed > pageRect.bottom())
        return { y, startStorageIndex, true };

    QString qtyText = (startStorageIndex == 0)
                          ? QString("%1 db").arg(aggregatedMaterial.totalQty)
                          : QString("...");

    const MaterialMaster* master =
        MaterialRegistry::instance().findById(aggregatedMaterial.materialId);

    QString name = master ? master->toReportLabel() : "?";

    auto headerText = QString("%1 (%2)").arg(name).arg(qtyText);

    painter.drawText(QRectF(leftMargin, y, pageRect.width(), needed),
                     Qt::AlignLeft, headerText);

    y += needed;

   // --- STORAGE SOROK ---

    QFont smallFont = painter.font();
    smallFont.setPointSizeF(smallFont.pointSizeF() * 0.85);
    smallFont.setItalic(true);

    painter.save();
    painter.setFont(smallFont);

    qreal storageLineH = fm.height() * 1.2;

    for (int i = startStorageIndex; i < aggregatedMaterial.storages.size(); ++i)
    {
        const auto& ste = aggregatedMaterial.storages[i];

        if (y + storageLineH > pageRect.bottom())
        {
            painter.restore();
            return { y, i, true };   // itt szakadt félbe
        }

        QString ld = ste.lastSeenAt.toString("yyyy.MM.dd HH:mm");

        QString q = ste.componentCount==1
                        ?QString("%1 db").arg(ste.packQuantity)
                        :QString("%1 db ×%2 = %3 db")
                            .arg(ste.packQuantity)
                            .arg(ste.componentCount)
                            .arg(ste.packQuantity * ste.componentCount);

        QString line = QString("→ %1[%2]: %3  → %4  @%5")
                           .arg(ste.humanName)
                           .arg(ste.logisticBarcode)
                           .arg(ste.packBarcode)
                           .arg(q)
                           .arg(ld);

        painter.drawText(QRectF(leftMargin + 25, y, pageRect.width(), storageLineH),
                         Qt::AlignLeft,line);

        y += storageLineH;
    }

    painter.restore();

    RowResult r;
    r.newY = y;
    r.nextStorageIndex = aggregatedMaterial.storages.size();
    r.pageBreakNeeded = false;

    return r;
}


} // namespace
