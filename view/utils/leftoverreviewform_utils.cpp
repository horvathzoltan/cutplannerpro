#include "leftoverreviewform_utils.h"
#include <QFontMetrics>
#include <model/registries/storageregistry.h>
#include "common/barcodepainter.h"   // ahol a BarcodePainter namespace van

namespace LeftoverReviewFormUtils {

static void drawCheckbox(QPainter& painter, const QPointF& topLeft, int cbSize)
{
    //const qreal size = 48.0;
    painter.drawRect(QRectF(topLeft, QSizeF(cbSize, cbSize)));
}

void formatReviewFormPdf(QPainter& painter,
                         QPdfWriter& writer,
                         const QRectF& pageRect,
                         const QVector<LeftoverStockEntry>& entries,
                         int rowsPerPage)
{
    const qreal margin = 40.0;
    int cbSize = 48.0;
    int cbGap = 60;
    int barcodeGap = 60;
    int barcodeHeight = 80;   // <<< új paraméter, szabadon állítható

    qreal usableHeight = pageRect.height() - 2 * margin - (rowsPerPage - 1) * 10.0;
    qreal lineHeight = usableHeight / rowsPerPage;

    QFontMetrics fm(painter.font());


    // minimális cellamagasság a tartalom alapján
    qreal minContentHeight =
        //cbSize + cbGap + barcodeGap + barcodeHeight + fm.height() +
        (4 * fm.height() + 3 * 4) + 20;   // 20 = bcMarginBottom


    if (lineHeight < minContentHeight)
        lineHeight = minContentHeight;

    qreal y = pageRect.top() + margin;
    int rowCountOnPage = 0;


    for (int i = 0; i < entries.size(); ++i) {
        // MINDEN rajzolás előtt fix tollvastagság
        painter.setPen(QPen(Qt::black, 2.0));


        if (rowCountOnPage >= rowsPerPage) {
            writer.newPage();
            y = pageRect.top() + margin;
            rowCountOnPage = 0;
        }

        const LeftoverStockEntry& e = entries[i];

        QRectF rowRect(pageRect.left() + margin,
                       y,
                       pageRect.width() - 2 * margin,
                       lineHeight);

        painter.drawRect(rowRect);

        qreal w = rowRect.width();
        qreal leftW  = w * 0.30;
        qreal midW   = w * 0.40;
        qreal rightW = w * 0.30;

        QRectF leftCol (rowRect.left(), y, leftW,  lineHeight);
        QRectF midCol  (leftCol.right(), y, midW,  lineHeight);
        QRectF rightCol(midCol.right(),  y, rightW, lineHeight);

        qreal totalTextHeight = 4 * fm.height() + 3 * 4.0;

        // A középső szöveg baseline-ja → minden blokk innen indul
        qreal tyBase = midCol.top() + (midCol.height() - totalTextHeight) / 2 + fm.ascent();


        // --- BAL: NINCS MEG (–) ---

        // Biztonsági margók a vonalkód körül
        qreal bcMarginX = 30.0;   // bal/jobb margó
        qreal bcMarginBottom = 20.0;


        qreal cbOffsetX = 30.0;   // ugyanaz, mint bcMarginX

        QPointF leftCheckbox(
            leftCol.left() + cbOffsetX,
            tyBase - fm.ascent()
            );

        drawCheckbox(painter, leftCheckbox, cbSize);

        // Barcode a checkbox alatt, a checkbox méretéhez kötve
        QString codeMinus = e.barcode + "-";

        QRectF minusRect(
            leftCol.left() + bcMarginX,
            leftCheckbox.y() + cbSize + barcodeGap,
            leftCol.width() - 2 * bcMarginX,
            barcodeHeight
            );
        BarcodePainter::drawCode128(painter, codeMinus, minusRect);

        // Felirat a barcode alatt

        // NINCS MEG felirat a checkbox MELLÉ
        painter.drawText(
            leftCheckbox.x() + cbSize + cbGap,   // checkbox jobb oldala + padding
            leftCheckbox.y() + fm.ascent(),     // baseline → checkbox tetejéhez igazítva
            QStringLiteral("NINCS MEG")
            );


        // --- KÖZÉP: leftover adatok ---
        const MaterialMaster* mat = e.master();
        QString matName = mat ? mat->toDisplay() : QStringLiteral("(ismeretlen anyag)");

        const auto* storage = StorageRegistry::instance().findById(e.storageId);
        QString storageName = storage ? storage->name : QStringLiteral("—");

        QString line1 = e.barcode;
        QString line2 = QString("%1 mm").arg(e.availableLength_mm);
        QString line3 = matName;
        QString line4 = QStringLiteral("Tároló: %1").arg(storageName);

        QStringList lines = { line1, line2, line3, line4 };

        for (QString& s : lines) {
            if (s.length() > 30)
                s = s.left(30) + "…";
        }

        //4 sor szöveg → összmagasság:
        //qreal totalTextHeight = 4 * fm.height() + 3 * 4.0;
        qreal tx = midCol.left() + 40;

        // baseline kezdőpont: cella közepe - fél szövegmagasság
        qreal ty = midCol.top() + (midCol.height() - totalTextHeight) / 2 + fm.ascent();

        painter.drawText(tx, ty, line1);
        painter.drawText(tx, ty + fm.height() + 4.0, line2);
        painter.drawText(tx, ty + 2*(fm.height() + 4.0), line3);
        painter.drawText(tx, ty + 3*(fm.height() + 4.0), line4);


        // --- JOBB: MEGVAN (+) ---

        // Checkbox top = baseline - ascent
        QPointF rightCheckbox(
            rightCol.left() + cbOffsetX,
            tyBase - fm.ascent()
            );

        drawCheckbox(painter, rightCheckbox, cbSize);

        // Barcode a checkbox alatt, a checkbox méretéhez kötve
        QString codePlus = e.barcode + "+";

        QRectF plusRect(
            rightCol.left() + bcMarginX,
            rightCheckbox.y() + cbSize + barcodeGap,
            rightCol.width() - 2 * bcMarginX,
            barcodeHeight
            );
        BarcodePainter::drawCode128(painter, codePlus, plusRect);

        // MEGVAN felirat a checkbox MELLÉ
        painter.drawText(
            rightCheckbox.x() + cbSize + cbGap,  // checkbox jobb oldala + padding
            rightCheckbox.y() + fm.ascent(),    // baseline → checkbox tetejéhez igazítva
            QStringLiteral("MEGVAN")
            );


        y += lineHeight + 10.0;
        rowCountOnPage++;
    }
}

} // namespace LeftoverReviewFormUtils
