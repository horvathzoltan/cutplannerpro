#include "materialgrouprepository.h"
#include "common/csvimporter.h"
#include "model/material/materialgroup.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <common/filenamehelper.h>
#include <model/registries/materialregistry.h>

// --- Stage 1: Convert ---

std::optional<MaterialGroupRepository::MaterialGroupRow>
MaterialGroupRepository::convertRowToMaterialGroupRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 3) {
        qWarning() << "Invalid group row at line" << lineIndex;
        return std::nullopt;
    }

    MaterialGroupRow row {
        .groupKey = parts[0].trimmed(),
        .groupName = parts[1].trimmed(),
        .colorHex = parts[2].trimmed()
    };

    return row;
}

std::optional<MaterialGroupRepository::MaterialGroupMemberRow>
MaterialGroupRepository::convertRowToMaterialGroupMemberRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 2) {
        qWarning() << "Invalid member row at line" << lineIndex;
        return std::nullopt;
    }

    MaterialGroupMemberRow row {
        .groupKey = parts[0].trimmed(),
        .materialBarCode = parts[1].trimmed()
    };

    return row;
}

// --- Stage 2: Build ---

std::optional<MaterialGroup>
MaterialGroupRepository::buildMaterialGroupFromRow(const MaterialGroupRow& row, int lineIndex) {
    if (row.groupKey.isEmpty() || row.groupName.isEmpty()) {
        qWarning() << "Missing fields in group row at line" << lineIndex;
        return std::nullopt;
    }

    //MaterialGroup group(row.logicalId, row.displayName, row.colorHex);
    MaterialGroup group;
    group.id = QUuid::createUuid();
    group.name = row.groupName;
    group.barcode = row.groupKey;
    group.colorHex = row.colorHex;
    return group;
}

std::optional<QUuid>
MaterialGroupRepository::buildMaterialIdFromMemberRow(const MaterialGroupMemberRow& row, int lineIndex) {
    if (row.materialBarCode.isEmpty()) {
        qWarning() << "Missing barcode in member row at line" << lineIndex;
        return std::nullopt;
    }

    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarCode);
    if (!mat) {
        qWarning() << "⚠️ Ismeretlen anyag barcode:" << row.materialBarCode << "(csoport:" << row.groupKey << ")";
    }

    return mat ? std::make_optional(mat->id) : std::nullopt;
}

// --- Stage 3: Load & Assemble ---

QVector<MaterialGroupRepository::MaterialGroupRow>
MaterialGroupRepository::loadGroupRows(const QString& filepath) {
    //const auto csv = CsvReader::read(filepath);
    //return CsvImporter::processCsvRows<MaterialGroupRepository::MaterialGroupRow>(csv, convertRowToMaterialGroupRow);
    return CsvReader::readAndConvert<MaterialGroupRow>(filepath, convertRowToMaterialGroupRow);
}

QVector<MaterialGroupRepository::MaterialGroupMemberRow>
MaterialGroupRepository::loadMemberRows(const QString& filepath) {
    //const auto csv = CsvReader::read(filepath);
    //return CsvImporter::processCsvRows<MaterialGroupRepository::MaterialGroupMemberRow>(csv, convertRowToMaterialGroupMemberRow);
    return CsvReader::readAndConvert<MaterialGroupMemberRow>(filepath, convertRowToMaterialGroupMemberRow);
}

void MaterialGroupRepository::buildIntoMaterialGroup(MaterialGroup* group, const QUuid& materialId) {
    if (! group) return;
    if (materialId.isNull()) {
        qWarning() << "Invalid material ID for group";
        return;
    }
    group->addMaterial(materialId);
}

// --- Entry Point ---

bool MaterialGroupRepository::loadFromCsv(MaterialGroupRegistry& registry) {    
    const auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString metaPath = helper.getGroupCsvFile();           // materialgroups.csv
    const QString membersPath = helper.getGroupMembersCsvFile(); // materialgroup_members.csv

    const auto groupRows = loadGroupRows(metaPath);
    const auto memberRows = loadMemberRows(membersPath);

    QMap<QString, MaterialGroup> groupMap;

    for (int i = 0; i < groupRows.size(); ++i) {
        const auto& row = groupRows[i];

        std::optional<MaterialGroup> groupOpt =
            buildMaterialGroupFromRow(row, i + 1);
        if (groupOpt.has_value()) {
            if (groupMap.contains(row.groupKey)) {
                qWarning() << "⚠️ Duplikált csoport:" << row.groupKey << "a sorban:" << i + 1;
            }

            groupMap.insert(row.groupKey, groupOpt.value());
        }
    }

    for (int i = 0; i < memberRows.size(); ++i) {
        const auto& row = memberRows[i];

        if (!groupMap.contains(row.groupKey)){
            qWarning() << "⚠️ Nem definiált csoport:" << row.groupKey << "a sorban:" << i + 1;
            continue;
        }

        auto materialId = buildMaterialIdFromMemberRow(row, i+1);

        if(!materialId.has_value()) {
            qWarning() << "⚠️ Ismeretlen anyag barcode:" << row.materialBarCode << "(csoport:" << row.groupKey << ")";
            continue;
        }

        MaterialGroup& group = groupMap[row.groupKey];
        buildIntoMaterialGroup(&group, materialId.value());
    }

    for (auto it = groupMap.constBegin(); it != groupMap.constEnd(); ++it) {
        registry.registerGroup(it.value());
    }

    return true;
}

// #include "materialgrouprepository.h"
// #include "common/tableutils/colorutils.h"
// #include "../material/materialgroup.h"
// #include "../registries/materialregistry.h"
// #include "../material/materialmaster.h"
// #include <QFile>
// #include <QTextStream>
// #include <QStringList>
// #include <QDebug>
// #include <common/filehelper.h>
// #include <common/filenamehelper.h>
// #include "common/logger.h"

// bool MaterialGroupRepository::loadFromCsv(MaterialGroupRegistry& registry) {
//     const auto& helper = FileNameHelper::instance();
//     if (!helper.isInited()) return false;

//     const QString metaPath = helper.getGroupCsvFile();           // materialgroups.csv
//     const QString membersPath = helper.getGroupMembersCsvFile(); // materialgroup_members.csv

//     const auto groupMeta = loadGroupMeta(metaPath);
//     const auto groupMembers = loadGroupMembers(membersPath);

//     if (groupMeta.isEmpty()) {
//         zWarning("❌ A materialgroups.csv fájl üres vagy hibás.");
//         return false;
//     }

//     for (const auto& groupKey : groupMeta.keys()) {
//         const auto& meta = groupMeta[groupKey];
//         MaterialGroup group;
//         group.id = QUuid::createUuid();
//         group.name = meta.displayName;
//         group.colorHex = meta.colorHex;

//         const auto barcodes = groupMembers.values(groupKey);
//         for (const auto& barcode : barcodes) {
//             const auto* mat = MaterialRegistry::instance().findByBarcode(barcode.trimmed());
//             if (!mat) {
//                 qWarning() << "⚠️ Ismeretlen anyag barcode:" << barcode << "(csoport:" << groupKey << ")";
//                 continue;
//             }
//             group.materialIds.append(mat->id);
//         }

//         registry.registerGroup(group);
//     }

//     // const QVector<MaterialGroup> groups = loadFromCsv_private(filepath);
//     // if (groups.isEmpty()) {
//     //     qWarning("⚠️ A groups.csv fájl üres vagy hibás formátumú.");
//     //     return false;
//     // }

//     // registry.clearAll(); // 🔄 Korábbi csoportok törlése
//     // for (const auto& g : groups)
//     //     registry.registerGroup(g);

//     return true;
// }

// QVector<MaterialGroup>
// MaterialGroupRepository::loadFromCsv_private(const QString& filepath) {
//     QFile file(filepath);
//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning() << "❌ Nem sikerült megnyitni a fájlt:" << filepath;
//         return {};
//     }

//     QTextStream in(&file);
//     in.setEncoding(QStringConverter::Utf8);

//     const auto rows = FileHelper::parseCSV(&in, ';');
//     if (rows.size() <= 1) {
//         qWarning() << "⚠️ A fájl üres vagy csak fejlécet tartalmaz.";
//         return {};
//     }

//     QMap<QString, MaterialGroup> tempGroups;

//     for (int i = 1; i < rows.size(); ++i) {
//         const auto rowOpt = convertRowToMaterialGroupRow(rows[i], i + 1);
//         if (!rowOpt.has_value()) continue;

//         const auto& row = rowOpt.value();
//         const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
//         if (!mat) {
//             qWarning() << QString("⚠️ Sor %1: ismeretlen barcode '%2'")
//                               .arg(i + 1).arg(row.barcode);
//             continue;
//         }

//         if (!tempGroups.contains(row.logicalId)) {
//             auto groupOpt = buildMaterialGroupFromRow(row, i + 1);
//             if (!groupOpt.has_value()) continue;
//             tempGroups[row.logicalId] = groupOpt.value();
//         } else {
//             tempGroups[row.logicalId].materialIds.append(mat->id);
//         }
//     }

//     return tempGroups.values().toVector();
// }


// std::optional<MaterialGroupRepository::MaterialGroupRow>
// MaterialGroupRepository::convertRowToMaterialGroupRow(const QVector<QString>& parts, int lineIndex) {
//     if (parts.size() < 3) {
//         qWarning() << QString("⚠️ Sor %1: kevés oszlop").arg(lineIndex);
//         return std::nullopt;
//     }

//     MaterialGroupRow row;
//     row.logicalId   = parts[0].trimmed();
//     row.displayName = parts[1].trimmed();
//     row.barcode     = parts[2].trimmed();

//     if (row.logicalId.isEmpty() || row.displayName.isEmpty() || row.barcode.isEmpty()) {
//         qWarning() << QString("⚠️ Sor %1: hiányzó kötelező mezők").arg(lineIndex);
//         return std::nullopt;
//     }

//     if (parts.size() > 3) {
//         const QString colorCandidate = parts[3].trimmed();
//         if (ColorUtils::validateColorHex(colorCandidate)) {
//             row.colorHex = colorCandidate;
//         } else {
//             qWarning() << QString("⚠️ Sor %1: hibás színkód '%2'")
//                               .arg(lineIndex).arg(colorCandidate);
//         }
//     }

//     return row;
// }

// std::optional<MaterialGroup>
// MaterialGroupRepository::buildMaterialGroupFromRow(const MaterialGroupRow& row, int lineIndex) {
//     const auto* mat = MaterialRegistry::instance().findByBarcode(row.barcode);
//     if (!mat) {
//         qWarning() << QString("⚠️ Sor %1: ismeretlen barcode '%2'")
//                           .arg(lineIndex).arg(row.barcode);
//         return std::nullopt;
//     }

//     MaterialGroup g;
//     g.groupId = QUuid::createUuid();
//     g.name     = row.displayName;
//     g.colorHex = row.colorHex;
//     g.materialIds.append(mat->id);

//     return g;
// }



