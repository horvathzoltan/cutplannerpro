#include "materialgrouprepository.h"
#include "common/tableutils/colorutils.h"
#include "../materialgroup.h"
#include "../registries/materialregistry.h"
#include "../materialmaster.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <common/filehelper.h>
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

    registry.clearAll(); // 🔄 Korábbi csoportok törlése
    for (const auto& g : groups)
        registry.registerGroup(g);

    return true;
}

QVector<MaterialGroup>
MaterialGroupRepository::loadFromCsv_private(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a fájlt:" << filepath;
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    const auto rows = FileHelper::parseCSV(&in, ';');
    if (rows.size() <= 1) {
        qWarning() << "⚠️ A fájl üres vagy csak fejlécet tartalmaz.";
        return {};
    }

    QMap<QString, MaterialGroup> tempGroups;

    for (int i = 1; i < rows.size(); ++i) {
        const auto rowOpt = convertRowToMaterialGroupRow(rows[i], i + 1);
        if (!rowOpt.has_value()) continue;

        const auto& row = rowOpt.value();
        const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
        if (!mat) {
            qWarning() << QString("⚠️ Sor %1: ismeretlen barcode '%2'")
                              .arg(i + 1).arg(row.barcode);
            continue;
        }

        if (!tempGroups.contains(row.logicalId)) {
            auto groupOpt = buildMaterialGroupFromRow(row, i + 1);
            if (!groupOpt.has_value()) continue;
            tempGroups[row.logicalId] = groupOpt.value();
        } else {
            tempGroups[row.logicalId].materialIds.append(mat->id);
        }
    }

    return tempGroups.values().toVector();
}






std::optional<MaterialGroupRepository::MaterialGroupRow>
MaterialGroupRepository::convertRowToMaterialGroupRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 3) {
        qWarning() << QString("⚠️ Sor %1: kevés oszlop").arg(lineIndex);
        return std::nullopt;
    }

    MaterialGroupRow row;
    row.logicalId   = parts[0].trimmed();
    row.displayName = parts[1].trimmed();
    row.barcode     = parts[2].trimmed();

    if (row.logicalId.isEmpty() || row.displayName.isEmpty() || row.barcode.isEmpty()) {
        qWarning() << QString("⚠️ Sor %1: hiányzó kötelező mezők").arg(lineIndex);
        return std::nullopt;
    }

    if (parts.size() > 3) {
        const QString colorCandidate = parts[3].trimmed();
        if (ColorUtils::validateColorHex(colorCandidate)) {
            row.colorHex = colorCandidate;
        } else {
            qWarning() << QString("⚠️ Sor %1: hibás színkód '%2'")
                              .arg(lineIndex).arg(colorCandidate);
        }
    }

    return row;
}

std::optional<MaterialGroup>
MaterialGroupRepository::buildMaterialGroupFromRow(const MaterialGroupRow& row, int lineIndex) {
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
    if (!mat) {
        qWarning() << QString("⚠️ Sor %1: ismeretlen barcode '%2'")
                          .arg(lineIndex).arg(row.barcode);
        return std::nullopt;
    }

    MaterialGroup g;
    g.groupId = QUuid::createUuid();
    g.name     = row.displayName;
    g.colorHex = row.colorHex;
    g.materialIds.append(mat->id);

    return g;
}



// QVector<MaterialGroup> MaterialGroupRepository::loadFromCsv_private(const QString& filepath) {
//     QVector<MaterialGroup> result;
//     QFile file(filepath);

//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning() << "❌ Nem sikerült megnyitni a CSV-t:" << filepath;
//         return result;
//     }

//     QTextStream in(&file);
//     QMap<QString, MaterialGroup> tempGroups;
//     bool isFirstLine = true;

//     while (!in.atEnd()) {
//         const QString line = in.readLine().trimmed();
//         if (line.isEmpty()) continue;
//         if (isFirstLine) { isFirstLine = false; continue; }

//         const QStringList parts = line.split(';');
//         if (parts.size() < 3) continue;

//         const QString logicalId     = parts[0].trimmed();
//         const QString displayName   = parts[1].trimmed();
//         const QString barcode       = parts[2].trimmed();

//         // 🎨 Színkód feldolgozása
//         QString colorHex;
//         bool hasColor = (parts.size() >= 4 && !parts[3].trimmed().isEmpty());
//         if (hasColor) {
//             const QString tempColor = parts[3].trimmed();
//             if (ColorUtils::validateColorHex(tempColor)) {
//                 colorHex = tempColor;
//             } else {
//                 qWarning() << "⚠️ Hibás színformátum a groups.csv fájlban:" << tempColor;
//             }
//         }

//         const auto optMat = MaterialRegistry::instance().findByBarcode(barcode);
//         if (!optMat) {
//             qWarning() << "⚠️ Hiányzó anyag barcode alapján:" << barcode;
//             continue;
//         }

//         if (!tempGroups.contains(logicalId)) {
//             MaterialGroup g;
//             g.groupId = QUuid::createUuid();
//             g.name = displayName;
//             g.colorHex = colorHex;
//             tempGroups[logicalId] = g;
//         }

//         tempGroups[logicalId].materialIds.append(optMat->id);
//     }

//     result = tempGroups.values().toVector();
//     return result;
// }

