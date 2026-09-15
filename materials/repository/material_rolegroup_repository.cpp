#include "materials/repository/material_rolegroup_repository.h"
#include "common/filehelper.h"
#include "common/filenamehelper.h"
#include "materials/registry/material_registry.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "common/logger.h"

// --- Stage 1: Convert ---

std::optional<MaterialRoleGroupRepository::RoleGroupRow>
MaterialRoleGroupRepository::convertRowToRoleGroupRow(
    const QVector<QString>& parts, CsvReader::FileContext& ctx)
{
    if (parts.size() < 2) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: hibás parent sor (legalább 2 mező kell).");
        return std::nullopt;
    }

    RoleGroupRow row {
        .groupKey = parts[0].trimmed(),
        .groupName = parts[1].trimmed()
    };

    return row;
}

std::optional<MaterialRoleGroupRepository::RoleGroupMemberRow>
MaterialRoleGroupRepository::convertMsffMemberRow(
    const QVector<QString>& parts, CsvReader::FileContext& ctx)
{
    if (parts.size() < 1) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: üres member sor.");
        return std::nullopt;
    }

    RoleGroupMemberRow row {
        .materialBarCode = parts[0].trimmed()
    };

    return row;
}

// --- Stage 2: Build ---

std::optional<MaterialRoleGroup>
MaterialRoleGroupRepository::buildRoleGroupFromRow(
    const RoleGroupRow& row, CsvReader::FileContext& ctx)
{
    if (row.groupKey.isEmpty() || row.groupName.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: parent sor hiányos (groupKey/groupName).");
        return std::nullopt;
    }

    MaterialRoleGroup group;
    group.id = QUuid::createUuid();
    group.barcode = row.groupKey;
    group.name = row.groupName;

    return group;
}

std::optional<QUuid>
MaterialRoleGroupRepository::buildMaterialIdFromMemberRow(
    const RoleGroupMemberRow& row, CsvReader::FileContext& ctx)
{
    if (row.materialBarCode.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: üres materialBarCode.");
        return std::nullopt;
    }

    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarCode);
    if (!mat) {
        zWarning("⚠️ MSFF: Ismeretlen anyag barcode:" +  row.materialBarCode);
        return std::nullopt;
    }

    return mat->id;
}

// --- Stage 3: Load & Assemble ---

QList<QList<QVector<QString>>>
MaterialRoleGroupRepository::readMsffSections(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        zWarning("❌ Nem sikerült megnyitni az MSFF fájlt:" + filepath);
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    // 1) szeparátor detektálás
    auto sepResult = FileHelper::detectSeparatorMsff(&in);
    if (sepResult.hasError || sepResult.separator.isNull()) {
        zWarning( "❌ MSFF: szeparátor detektálás sikertelen.");
        return {};
    }

    // 2) vissza a fájl elejére
    file.seek(0);
    in.seek(0);

    // 3) teljes MSFF beolvasása
    auto allRows = FileHelper::parseCSV(&in, sepResult.separator, true);

    // 4) szekciók szétválasztása
    return FileHelper::splitSections(allRows, sepResult.headerLineCount);
}

bool MaterialRoleGroupRepository::parseMsffSections(
    const QList<QList<QVector<QString>>>& sections,
    MaterialRoleGroupRegistry& registry)
{
    for (const auto& sec : sections) {
        if (sec.isEmpty()) continue;

        // --- Parent sor ---
        CsvReader::FileContext ctx("msff-parent");
        auto maybeRow = convertRowToRoleGroupRow(sec[0], ctx);
        if (!maybeRow.has_value()) {
            zWarning( "❌ MSFF: hibás parent sor.");
            return false;
        }

        auto maybeGroup = buildRoleGroupFromRow(maybeRow.value(), ctx);
        if (!maybeGroup.has_value()) {
            zWarning( "❌ MSFF: hibás group objektum.");
            return false;
        }

        MaterialRoleGroup group = maybeGroup.value();

        // --- Child sorok ---
        for (int i = 1; i < sec.size(); ++i) {
            CsvReader::FileContext ctx2("msff-member");
            auto maybeMember = convertMsffMemberRow(sec[i], ctx2);
            if (!maybeMember.has_value()) {
                zWarning("❌ MSFF: hibás member sor.");
                return false;
            }

            auto maybeMatId = buildMaterialIdFromMemberRow(maybeMember.value(), ctx2);
            if (maybeMatId.has_value()) {
                group.addMaterial(maybeMatId.value());
            }
        }

        if (registry.containsBarcode(group.barcode)) {
            zWarning("⚠️ MSFF: duplikált szerepkör-csoport:" + group.barcode);
        }

        registry.registerGroup(group);
    }

    return true;
}

// --- Entry Point ---

bool MaterialRoleGroupRepository::loadFromMsff(MaterialRoleGroupRegistry& registry)
{
    const auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString path = helper.getMaterialRoleGroupMsffFile(); // materialrolegroups.msff

    auto sections = readMsffSections(path);
    if (sections.isEmpty()) {
        zWarning("❌ MSFF: üres vagy hibás fájl.");
        return false;
    }

    return parseMsffSections(sections, registry);
}
