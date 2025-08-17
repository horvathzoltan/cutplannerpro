#include "archivedwasteutils.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

void ArchivedWasteUtils::exportToCSV(const QVector<ArchivedWasteEntry>& entries, const QString& folderPath)
{
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString path = folderPath.isEmpty() ? "exports/" : folderPath;
    QDir().mkpath(path); // létrehozza a mappát, ha nem létezik

    QFile file(path + QString("/archived_waste_%1.csv").arg(timestamp));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "MaterialId,WasteLength,Source,CreatedAt,Group,Barcode,Note,CutPlanId\n";
    for (const auto& e : entries)
        out << e.toCSVLine() << "\n";

    file.close();
}
