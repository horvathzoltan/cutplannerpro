#pragma once

#include <QString>
#include <QVector>
#include "model/archivedwasteentry.h"

namespace ArchivedWasteUtils {
void exportToCSV(const QVector<ArchivedWasteEntry>& entries, const QString& folderPath = {});
}
