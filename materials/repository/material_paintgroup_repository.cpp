#include "materials/repository/material_paintgroup_repository.h"
#include "common/filehelper.h"
#include "common/filenamehelper.h"
#include "materials/registry/material_registry.h"
//#include "materials/model/material_paintgeometry_type_utils.h"
#include "common/logger.h"

#include <QFile>
#include <QTextStream>

// --- Stage 1: Convert ---

std::optional<MaterialPaintGroupRepository::PaintGroupRow>
MaterialPaintGroupRepository::convertRowToPaintGroupRow(
    const QVector<QString>& parts, CsvReader::FileContext& ctx)
{
    // groupKey;groupName;geometry;dimA;dimB;extraCut
    if (parts.size() < 4) {
        ctx.addError(ctx.currentLineNumber(),
                     "❌ MSFF: hibás parent sor (legalább 4 mező kell: groupKey;groupName;geometry;dimA).");
        return std::nullopt;
    }

    PaintGroupRow row;
    row.groupKey  = parts[0].trimmed();
    row.groupName = parts[1].trimmed();
    row.geometry  = PaintGeometryTypeUtils::parse(parts[2].trimmed());

    // dimA kötelező
    row.dimA_mm = parts[3].trimmed().toInt();

    // dimB opcionális
    if (parts.size() > 4 && !parts[4].trimmed().isEmpty())
        row.dimB_mm = parts[4].trimmed().toInt();

    // extraCut opcionális
    if (parts.size() > 5 && !parts[5].trimmed().isEmpty())
        row.extraCut_mm = parts[5].trimmed().toInt();

    return row;
}

std::optional<MaterialPaintGroupRepository::PaintGroupMemberRow>
MaterialPaintGroupRepository::convertMsffMemberRow(
    const QVector<QString>& parts, CsvReader::FileContext& ctx)
{
    if (parts.size() < 1) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: üres member sor.");
        return std::nullopt;
    }

    PaintGroupMemberRow row {
        .materialBarCode = parts[0].trimmed()
    };

    return row;
}

// --- Stage 2: Build ---

std::optional<MaterialPaintGroup>
MaterialPaintGroupRepository::buildPaintGroupFromRow(
    const PaintGroupRow& row, CsvReader::FileContext& ctx)
{
    if (row.groupKey.isEmpty() || row.groupName.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(),
                     "❌ MSFF: parent sor hiányos (groupKey/groupName).");
        return std::nullopt;
    }

    MaterialPaintGroup group;
    group.id      = QUuid::createUuid();
    group.barcode = row.groupKey;
    group.name    = row.groupName;

    group.geometry = row.geometry;
    group.dimA_mm  = row.dimA_mm;
    group.dimB_mm  = row.dimB_mm;
    group.extraCut_mm = row.extraCut_mm;

    return group;
}

std::optional<QUuid>
MaterialPaintGroupRepository::buildMaterialIdFromMemberRow(
    const PaintGroupMemberRow& row, CsvReader::FileContext& ctx)
{
    if (row.materialBarCode.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: üres materialBarCode.");
        return std::nullopt;
    }

    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarCode);
    if (!mat) {
        zWarning("⚠️ MSFF: Ismeretlen anyag barcode:" + row.materialBarCode);
        return std::nullopt;
    }

    return mat->id;
}

// --- Stage 3: Load & Assemble ---

QList<QList<QVector<QString>>>
MaterialPaintGroupRepository::readMsffSections(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        zWarning("❌ Nem sikerült megnyitni az MSFF fájlt:" + filepath);
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    auto sepResult = FileHelper::detectSeparatorMsff(&in);
    if (sepResult.hasError || sepResult.separator.isNull()) {
        zWarning("❌ MSFF: szeparátor detektálás sikertelen.");
        return {};
    }

    file.seek(0);
    in.seek(0);

    auto allRows = FileHelper::parseCSV(&in, sepResult.separator, true);
    return FileHelper::splitSections(allRows, sepResult.headerLineCount);
}

bool MaterialPaintGroupRepository::parseMsffSections(
    const QList<QList<QVector<QString>>>& sections,
    MaterialPaintGroupRegistry& registry)
{
    for (const auto& sec : sections) {
        if (sec.isEmpty()) continue;

        CsvReader::FileContext ctx("msff-paint-parent");
        auto maybeRow = convertRowToPaintGroupRow(sec[0], ctx);
        if (!maybeRow.has_value()) {
            zWarning("❌ MSFF: hibás festési parent sor.");
            return false;
        }

        auto maybeGroup = buildPaintGroupFromRow(maybeRow.value(), ctx);
        if (!maybeGroup.has_value()) {
            zWarning("❌ MSFF: hibás festési group objektum.");
            return false;
        }

        MaterialPaintGroup group = maybeGroup.value();

        // Child sorok
        for (int i = 1; i < sec.size(); ++i) {
            CsvReader::FileContext ctx2("msff-paint-member");
            auto maybeMember = convertMsffMemberRow(sec[i], ctx2);
            if (!maybeMember.has_value()) {
                zWarning("❌ MSFF: hibás festési member sor.");
                return false;
            }

            auto maybeMatId = buildMaterialIdFromMemberRow(maybeMember.value(), ctx2);
            if (maybeMatId.has_value()) {
                group.addMaterial(maybeMatId.value());
            }
        }

        if (registry.containsBarcode(group.barcode)) {
            zWarning("⚠️ MSFF: duplikált festési csoport:" + group.barcode);
        }

        registry.registerGroup(group);
    }

    return true;
}

// --- Entry Point ---

bool MaterialPaintGroupRepository::loadFromMsff(MaterialPaintGroupRegistry& registry)
{
    const auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString path = helper.getMaterialPaintGroupMsffFile();

    auto sections = readMsffSections(path);
    if (sections.isEmpty()) {
        zWarning("❌ MSFF: üres vagy hibás festési fájl.");
        return false;
    }

    return parseMsffSections(sections, registry);
}
