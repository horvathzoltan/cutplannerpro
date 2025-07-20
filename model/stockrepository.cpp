#include "materialregistry.h"
#include "stockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <common/filenamehelper.h>

bool StockRepository::loadFromCSV(StockRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    QString path = helper.getStockCsvFile();
    if (path.isEmpty()) {
        qWarning("Nincs elérhető stock.csv fájl");
        return false;
    }

    QVector<StockEntry> entries = loadFromCSV_private(path);
    if (entries.isEmpty()) {
        qWarning("A stock.csv fájl üres vagy hibás sorokat tartalmaz.");
        return false;
    }

    registry.clear(); // 🔄 Korábbi készlet törlése
    for (const auto& entry : entries)
        registry.add(entry);

    return true;
}

QVector<StockEntry> StockRepository::loadFromCSV_private(const QString& filepath) {
    QVector<StockEntry> result;
    QFile file(filepath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a stock fájlt:" << filepath;
        return result;
    }

    QTextStream in(&file);
    bool firstLine = true;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (firstLine) { firstLine = false; continue; }

        const QStringList cols = line.split(';');
        if (cols.size() < 2) continue;

        QString barcode = cols[0].trimmed();
        bool okQty = false;
        int quantity = cols[1].toInt(&okQty);
        if (!okQty || quantity <= 0) continue;

        const MaterialMaster* mat = MaterialRegistry::instance().findByBarcode(barcode);
        if (!mat) {
            qWarning() << "⚠️ Hiányzó anyag barcode alapján:" << barcode;
            continue;
        }

        StockEntry entry;
        entry.materialId = mat->id;
        entry.quantity   = quantity;
        result.append(entry);
    }

    file.close();
    return result;
}
