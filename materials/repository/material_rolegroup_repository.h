#pragma once

#include <QString>
#include <QList>
#include <optional>

#include "materials/model/material_rolegroup.h"
#include "materials/registry/material_rolegroup_registry.h"
#include "common/csvimporter.h"

class MaterialRoleGroupRepository {
public:
    static bool loadFromMsff(MaterialRoleGroupRegistry& registry);

private:
    struct RoleGroupRow {
        QString groupKey;
        QString groupName;
    };

    struct RoleGroupMemberRow {
        QString materialBarCode;
    };

    // --- Stage 1: Convert ---
    static std::optional<RoleGroupRow>
    convertRowToRoleGroupRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    static std::optional<RoleGroupMemberRow>
    convertMsffMemberRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    // --- Stage 2: Build ---
    static std::optional<MaterialRoleGroup>
    buildRoleGroupFromRow(const RoleGroupRow& row, CsvReader::FileContext& ctx);

    static std::optional<QUuid>
    buildMaterialIdFromMemberRow(const RoleGroupMemberRow& row, CsvReader::FileContext& ctx);

    // --- Stage 3: Load & Assemble ---
    static QList<QList<QVector<QString>>>
    readMsffSections(const QString& filepath);

    static bool parseMsffSections(const QList<QList<QVector<QString>>>& sections,
                                  MaterialRoleGroupRegistry& registry);
};
