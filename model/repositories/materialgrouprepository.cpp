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
    return CsvReader::readAndConvert<MaterialGroupRow>(filepath, convertRowToMaterialGroupRow);
}

QVector<MaterialGroupRepository::MaterialGroupMemberRow>
MaterialGroupRepository::loadMemberRows(const QString& filepath) {
    return CsvReader::readAndConvert<MaterialGroupMemberRow>(filepath, convertRowToMaterialGroupMemberRow);
}

void MaterialGroupRepository::addMaterialToGroup(MaterialGroup* group, const QUuid& materialId) {
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
        addMaterialToGroup(&group, materialId.value());
    }

    for (auto it = groupMap.constBegin(); it != groupMap.constEnd(); ++it) {
        registry.registerGroup(it.value());
    }

    return true;
}

