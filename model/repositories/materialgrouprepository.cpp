#include "materialgrouprepository.h"
#include "common/colorutils.h"
#include "../materialgroup.h"
#include "../registries/materialregistry.h"
#include "../materialmaster.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <common/filenamehelper.h>

bool MaterialGroupRepository::loadFromCsv(MaterialGroupRegistry& registry) {
    const auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString filepath = helper.getGroupCsvFile(); // 📁 például "groups.csv"
    if (filepath.isEmpty()) {
        qWarning("❌ Nem található a groups.csv fájl.");
        return false;
    }

    const QVector<MaterialGroup> groups = loadFromCsv_private(filepath);
    if (groups.isEmpty()) {
        qWarning("⚠️ A groups.csv fájl üres vagy hibás formátumú.");
        return false;
    }

    registry.clear(); // 🔄 Korábbi csoportok törlése
    for (const auto& g : groups)
        registry.addGroup(g);

    return true;
}

QVector<MaterialGroup> MaterialGroupRepository::loadFromCsv_private(const QString& filepath) {
    QVector<MaterialGroup> result;
    QFile file(filepath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a CSV-t:" << filepath;
        return result;
    }

    QTextStream in(&file);
    QMap<QString, MaterialGroup> tempGroups;
    bool isFirstLine = true;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (isFirstLine) { isFirstLine = false; continue; }

        const QStringList parts = line.split(';');
        if (parts.size() < 3) continue;

        const QString logicalId     = parts[0].trimmed();
        const QString displayName   = parts[1].trimmed();
        const QString barcode       = parts[2].trimmed();

        // 🎨 Színkód feldolgozása
        QString colorHex;
        bool hasColor = (parts.size() >= 4 && !parts[3].trimmed().isEmpty());
        if (hasColor) {
            const QString tempColor = parts[3].trimmed();
            if (ColorUtils::validateColorHex(tempColor)) {
                colorHex = tempColor;
            } else {
                qWarning() << "⚠️ Hibás színformátum a groups.csv fájlban:" << tempColor;
            }
        }

        const auto optMat = MaterialRegistry::instance().findByBarcode(barcode);
        if (!optMat) {
            qWarning() << "⚠️ Hiányzó anyag barcode alapján:" << barcode;
            continue;
        }

        if (!tempGroups.contains(logicalId)) {
            MaterialGroup g;
            g.groupId = QUuid::createUuid();
            g.name = displayName;
            g.colorHex = colorHex;
            tempGroups[logicalId] = g;
        }

        tempGroups[logicalId].materialIds.append(optMat->id);
    }

    result = tempGroups.values().toVector();
    return result;
}

