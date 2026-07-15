#pragma once

#include <QPainter>
#include <QPdfWriter>
#include <QRectF>
#include <QVector>
#include <model/leftoverstockentry.h>

namespace LeftoverReviewFormUtils {

void formatReviewFormPdf(QPainter& painter,
                         QPdfWriter& writer,
                         const QRectF& pageRect,
                         const QVector<LeftoverStockEntry>& entries,
                         int rowsPerPage);

}
