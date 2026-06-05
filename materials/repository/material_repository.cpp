#include "materials/repository/material_repository.h"
#include <QFile>
#include <QTextStream>
#include <QUuid>

//#include "common/categoryutils.h"
#include "materials/repository/material_repository.h"
#include "materials/model/material_master.h"
#include "materials/model/material_type.h"
#include "model/crosssectionshape.h"
#include "materials/registry/material_registry.h"
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <common/filehelper.h>
#include <common/filenamehelper.h>
#include <common/csvimporter.h>
#include <common/color/namedcolor.h>
#include "materials/model/cutting_mode.h"
#include "materials/model/painting_mode.h"
#include "service/cutting/optimizer/optimizerconstants.h"
// bool MaterialRepository::loadFromCSV(MaterialRegistry& registry) {
//     auto& helper = FileNameHelper::instance();
//     if (!helper.isInited()) return false;

//     auto fn = helper.getMaterialCsvFile();
//     if (fn.isEmpty()) {
//         zWarning("Nem található a tesztadatok CSV fájlja.");
//         return false;
//     }

//     CsvReader::FileContext ctx(fn);
//     const QVector<MaterialMaster> loaded = loadFromCSV_private(ctx);
//     if (loaded.isEmpty()) {
//         zWarning("A materials.csv fájl üres vagy hibás formátumú.");
//         return false;
//     }

//     registry.setData(loaded); // 🔧 Itt történik az anyagregisztráció
//     return true;
// }

bool MaterialRepository::loadFromCSV(MaterialRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) {
        zWarning("❌ A FileNameHelper nincs inicializálva.");
        return false;
    }

    const QString fn = helper.getMaterialCsvFile();
    if (fn.isEmpty()) {
        zWarning("❌ Nem található a tesztadatok CSV fájlja.");
        return false;
    }

    CsvReader::FileContext ctx(fn);
    const QVector<MaterialMaster> loaded = loadFromCSV_private(ctx);

    // 🔍 Hibák loggolása, ha vannak
    if (ctx.hasErrors()) {
        zWarning(QString("⚠️ Hibák az importálás során (%1 sor):").arg(ctx.errorsSize()));
        zWarning(ctx.toString());
    }

    if (loaded.isEmpty()) {
        zWarning("❌ A materials.csv fájl üres vagy hibás formátumú.");
        return false;
    }

    registry.setData(loaded); // 🔧 Anyagregisztráció
    zInfo(QString("✅ %1 anyag sikeresen importálva a fájlból: %2").arg(loaded.size()).arg(fn));
    return true;
}


QVector<MaterialMaster>
MaterialRepository::loadFromCSV_private(CsvReader::FileContext& ctx) {
    return CsvReader::readAndConvert<MaterialMaster>(ctx, convertRowToMaterial, true);
}

std::optional<MaterialMaster>
MaterialRepository::convertRowToMaterial(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    const auto rowOpt = convertRowToMaterialRow(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildMaterialFromRow(rowOpt.value(), ctx);
}

std::optional<MaterialRepository::MaterialRow>
MaterialRepository::convertRowToMaterialRow(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    if (parts.size() < 11) {
        QString msg = L("⚠️ Kevés mező (legalább 9)");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    MaterialRow row;
    row.name       = parts[0].trimmed();
    row.barcode    = parts[1].trimmed();
    row.dim1       = parts[3].trimmed();
    row.dim2       = parts[4].trimmed();
    row.shapeStr   = parts[5].trimmed();
    row.machineId  = parts[6].trimmed();
    row.typeStr    = parts[7].trimmed();
    row.colorStr = parts[8].trimmed();
    row.cuttingMode = parts[9].trimmed();
    row.paintingMode = parts[10].trimmed();

    const QString lengthStr = parts[2].trimmed();
    bool okLength = false;
    row.stockLength = lengthStr.toDouble(&okLength);

    if (row.barcode.isEmpty() || !okLength || row.stockLength <= 0) {
        QString msg = L("⚠️ Érvénytelen barcode vagy hossz");
        ctx.addError(ctx.currentLineNumber(), msg);
        return std::nullopt;
    }

    // új mezők opcionális beolvasása
    if (parts.size() >= 12) row.trimStr            = parts[11].trimmed();
    if (parts.size() >= 13) row.minLeftOverStr     = parts[12].trimmed();
    if (parts.size() >= 14) row.scrapStr           = parts[13].trimmed();
    if (parts.size() >= 15) row.goodLeftOverMinStr = parts[14].trimmed();
    if (parts.size() >= 16) row.goodLeftOverMaxStr = parts[15].trimmed();
    if (parts.size() >= 17) row.externalCodeStr    = parts[16].trimmed();
    if (parts.size() >= 18) row.description    = parts[17].trimmed();

    return row;
}

std::optional<MaterialMaster>
MaterialRepository::buildMaterialFromRow(const MaterialRow& row, CsvReader::FileContext& ctx) {
    MaterialMaster m;
    m.id = QUuid::createUuid();
    m.name = row.name;
    m.barcode = row.barcode;
    m.stockLength_mm = row.stockLength;
    m.defaultMachineId = row.machineId;
    m.shape = CrossSectionShape::fromString(row.shapeStr);
    m.type = MaterialType::fromString(row.typeStr);
    m.cuttingMode = CuttingModeUtils::parse(row.cuttingMode);
    m.paintingMode = PaintingModeUtils::parse(row.paintingMode);

    if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Rectangular)) {
        bool okW = false, okH = false;
        double w = row.dim1.toDouble(&okW);
        double h = row.dim2.toDouble(&okH);
        if (!okW || !okH || w <= 0 || h <= 0) {
            QString msg = L("⚠️ Érvénytelen szélesség/magasság");
            ctx.addError(ctx.currentLineNumber(), msg);
        }
        m.size_mm = QSizeF(w, h);
    }
    else if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Round)) {
        bool okD = false;
        double d = row.dim1.toDouble(&okD);
        if (!okD || d <= 0) {
            QString msg = L("⚠️ Érvénytelen átmérő");
            ctx.addError(ctx.currentLineNumber(), msg);
        }
        m.diameter_mm = d;
    }

    // 🎨 Szín hozzárendelés – RAL, HEX vagy üres
    if (!row.colorStr.isEmpty()) {
        m.color = NamedColor(row.colorStr);
        if (!m.color.isValid()) {
            QString msg = L("⚠️ Ismeretlen színformátum: %1").arg( row.colorStr);
            ctx.addError(ctx.currentLineNumber(), msg);
        }
    } else {
        m.color = NamedColor(); // nincs festve
    }

    // új mezők parse + fallback
    bool okTrim, okMinLeft, okScrap, okGoodMin, okGoodMax;

    m.trim_mm              = row.trimStr.toInt(&okTrim);
    m.minLeftOver_mm       = row.minLeftOverStr.toInt(&okMinLeft);
    m.scrap_mm             = row.scrapStr.toInt(&okScrap);
    m.goodLeftOver_Min_mm  = row.goodLeftOverMinStr.toInt(&okGoodMin);
    m.goodLeftOver_Max_mm  = row.goodLeftOverMaxStr.toInt(&okGoodMax);

    if (!okTrim)        m.trim_mm             = OptimizerConstants_2::END_TRIM_MM;
    if (!okMinLeft)     m.minLeftOver_mm      = OptimizerConstants_2::MINIMUM_HULLO_MM;
    if (!okScrap)       m.scrap_mm            = OptimizerConstants_2::SELEJT_THRESHOLD;
    if (!okGoodMin)     m.goodLeftOver_Min_mm = OptimizerConstants_2::GOOD_LEFTOVER_MIN;
    if (!okGoodMax)     m.goodLeftOver_Max_mm = OptimizerConstants_2::GOOD_LEFTOVER_MAX;

    m.externalCode = row.externalCodeStr;
    m.description = row.description;

    return m;

}


