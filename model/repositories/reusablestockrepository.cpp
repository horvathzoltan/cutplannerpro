#include "reusablestockrepository.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "common/filenamehelper.h"
#include <model/registries/materialregistry.h>

bool ReusableStockRepository::loadFromCSV(ReusableStockRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    QString path = helper.getLeftoversCsvFile();
    if (path.isEmpty()) {
        qWarning("Nincs elérhető leftovers.csv fájl");
        return false;
    }

    QVector<ReusableStockEntry> entries = loadFromCSV_private(path);
    if (entries.isEmpty()) {
        qWarning("A leftovers.csv fájl üres vagy hibás sorokat tartalmaz.");
        return false;
    }

    registry.clear(); // 🔄 Korábbi készlet törlése
    for (const auto& entry : entries)
        registry.add(entry);

    return true;
}

QVector<ReusableStockEntry> ReusableStockRepository::loadFromCSV_private(const QString& filepath) {
    QVector<ReusableStockEntry> result;
    QFile file(filepath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a leftovers fájlt:" << filepath;
        return result;
    }

    QTextStream in(&file);
    bool firstLine = true;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (firstLine) { firstLine = false; continue; }

        const QStringList cols = line.split(';');
        if (cols.size() < 5) continue;

        QString materialBarCode = cols[0].trimmed();
        bool okQty = false;
        int availableLength_mm = cols[1].toInt(&okQty);
        if (!okQty || availableLength_mm <= 0) continue;

        QString sourceStr = cols[2].trimmed();
        LeftoverSource source = (sourceStr == "Optimization") ? LeftoverSource::Optimization : LeftoverSource::Manual;

        std::optional<int> optimizationId = std::nullopt;
        if (source == LeftoverSource::Optimization && cols.size() > 3) {
            bool okOptId = false;
            int optId = cols[3].toInt(&okOptId);
            if (okOptId) optimizationId = optId;
        }

        QString barcode = cols[4].trimmed();

        const MaterialMaster* mat = MaterialRegistry::instance().findByBarcode(materialBarCode);
        if (!mat) {
            qWarning() << "⚠️ Hiányzó anyag barcode alapján:" << materialBarCode;
            continue;
        }

        ReusableStockEntry entry;
        entry.materialId = mat->id;
        entry.availableLength_mm = availableLength_mm;
        entry.barcode = barcode;
        entry.source = source;
        entry.optimizationId = optimizationId;
        result.append(entry);
    }

    file.close();
    return result;
}
