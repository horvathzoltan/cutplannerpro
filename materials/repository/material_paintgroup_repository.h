#pragma once

#include <QString>
#include <QList>
#include <optional>

#include "materials/model/material_paintgroup.h"
#include "materials/registry/material_paintgroup_registry.h"
#include "common/csvimporter.h"

class MaterialPaintGroupRepository {
public:
    static bool loadFromMsff(MaterialPaintGroupRegistry& registry);

private:
    struct PaintGroupRow {
        QString groupKey;
        QString groupName;
        PaintGeometryType geometry;
        int dimA_mm;
        std::optional<int> dimB_mm;
        std::optional<int> extraCut_mm;
    };

    struct PaintGroupMemberRow {
        QString materialBarCode;
    };

    // Stage 1: Convert
    static std::optional<PaintGroupRow>
    convertRowToPaintGroupRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    static std::optional<PaintGroupMemberRow>
    convertMsffMemberRow(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    // Stage 2: Build
    static std::optional<MaterialPaintGroup>
    buildPaintGroupFromRow(const PaintGroupRow& row, CsvReader::FileContext& ctx);

    static std::optional<QUuid>
    buildMaterialIdFromMemberRow(const PaintGroupMemberRow& row, CsvReader::FileContext& ctx);

    // Stage 3: Load & Assemble
    static QList<QList<QVector<QString>>>
    readMsffSections(const QString& filepath);

    static bool parseMsffSections(const QList<QList<QVector<QString>>>& sections,
                                  MaterialPaintGroupRegistry& registry);
};
