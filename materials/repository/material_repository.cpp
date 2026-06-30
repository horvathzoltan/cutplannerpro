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
#include <materials/model/material_family_utils.h>
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

    //registry.applyFamilyDetection();

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

    const MaterialRow& row = rowOpt.value();
    // ⭐ LEVEL 1 VALIDÁCIÓ

    auto v1 = row.validate_level1(ctx);

    if (!v1.ok)
        return std::nullopt;

    return buildMaterialFromRow(row, v1, ctx);
}

std::optional<MaterialRepository::MaterialRow>
MaterialRepository::convertRowToMaterialRow(const QVector<QString>& parts, CsvReader::FileContext& ctx) {
    if (parts.size() < FIELD_COUNT) {
        ctx.addError(ctx.currentLineNumber(),
                     QString("❌ Kevés mező: %1 mező érkezett, de legalább %2 szükséges")
                         .arg(parts.size())
                         .arg(FIELD_COUNT));
        return std::nullopt;
    }

    if (parts.size() > FIELD_COUNT) {
        ctx.addError(ctx.currentLineNumber(),
                     QString("❌ Túl sok mező: %1 mező érkezett, de legfeljebb %2 lehet")
                         .arg(parts.size())
                         .arg(FIELD_COUNT));
        return std::nullopt;
    }

    MaterialRow row;
    row.name       = parts[0].trimmed();
    row.barcode    = parts[1].trimmed();
    row.stockLengthStr = parts[2].trimmed();
    row.dim1       = parts[3].trimmed();
    row.dim2       = parts[4].trimmed();
    row.shapeStr   = parts[5].trimmed();
    row.machineId  = parts[6].trimmed();
    row.typeStr    = parts[7].trimmed();
    row.colorStr   = parts[8].trimmed();
    row.surfaceStr = parts[9].trimmed();
    row.cuttingMode= parts[10].trimmed();
    row.paintingMode=parts[11].trimmed();
    row.trimStr            = parts[12].trimmed();
    row.minLeftOverStr     = parts[13].trimmed();
    row.scrapStr           = parts[14].trimmed();
    row.goodLeftOverMinStr = parts[15].trimmed();
    row.goodLeftOverMaxStr = parts[16].trimmed();
    row.externalCodeStr    = parts[17].trimmed();
    row.description        = parts[18].trimmed();
    row.familyStr          = parts[19].trimmed();

    return row;
}

MaterialRepository::MaterialRow::ValidatorResponse_level1
MaterialRepository::MaterialRow::validate_level1(CsvReader::FileContext& ctx) const
{
    MaterialRepository::MaterialRow::ValidatorResponse_level1 r(true);

    // 1) barcode kötelező
    if (barcode.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(), "⚠️ Üres barcode");
        return false;
    }

    // 2) stockLength szám-e?
    bool okLength = false;
    r.len = stockLengthStr.toDouble(&okLength);

    if (!okLength) {
        ctx.addError(ctx.currentLineNumber(), "⚠️ A stockLength mező nem szám");
        return false;
    }

    // 3) negatív hossz tiltott
    if (r.len < 0) {
        ctx.addError(ctx.currentLineNumber(), "⚠️ Negatív hossz nem megengedett");
        return false;
    }

    return r;
}


std::optional<MaterialMaster>
MaterialRepository::buildMaterialFromRow(const MaterialRow& row,
                                         const MaterialRepository::MaterialRow::ValidatorResponse_level1& v1,
                                         CsvReader::FileContext& ctx) {
    MaterialMaster m;
    m.id = QUuid::createUuid();
    m.name = row.name;
    m.barcode = row.barcode;
    m.stockLength_mm = v1.len;
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

    // ⭐ Felületminőség hozzárendelés (SM, FS, CS, MT, GL, ST)
    if (!row.surfaceStr.isEmpty()) {
        m.surface = SurfaceTypeUtils::fromCode(row.surfaceStr);
    } else {
        // ha üres → ipari default: Smooth
        m.surface = SurfaceType::Smooth;
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

    // family beolvasása
    m.family = MaterialFamilyUtils::fromString(row.familyStr);

    // // kötelező mező
    // if (m.family == MaterialFamily::Unknown) {
    //     ctx.addError(ctx.currentLineNumber(),
    //                  QString("❌ Érvénytelen vagy hiányzó family mező: '%1'")
    //                      .arg(row.familyStr));
    //     return std::nullopt;
    // }

    return m;

}

void MaterialRepository::exportCsv(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        zError(QString("❌ exportCsv: Nem sikerült megnyitni a fájlt írásra: %1").arg(path));
        return;
    }

    QTextStream out(&f);

    out << "name;barcode;stockLength;dim1;dim2;shape;machineId;type;color;surface;"
            "cuttingMode;paintingMode;trim;minLeftOver;scrap;goodLeftOverMin;"
            "goodLeftOverMax;externalCode;description;family\n";


    const auto& materials = MaterialRegistry::instance().readAll();

    int written = 0;

    for (const auto& m : materials) {
        out << m.name << ";"
            << m.barcode << ";"
            << m.stockLength_mm << ";";

        if (m.shape.isRound()) {
            out << m.diameter_mm << ";"
                << "" << ";";
        }
        else if (m.shape.isRectangular()) {
            out << m.size_mm.width() << ";"
                << m.size_mm.height() << ";";
        }
        else {
            out << "" << ";"
                << "" << ";";
        }

        out << m.shape.toString() << ";"
            << m.defaultMachineId << ";"
            << m.type.toString() << ";"
            << m.color.code() << ";"
            << SurfaceTypeUtils::toCode(m.surface) << ";"
            << CuttingModeUtils::toString(m.cuttingMode) << ";"
            << PaintingModeUtils::toString(m.paintingMode) << ";"
            << m.trim_mm << ";"
            << m.minLeftOver_mm << ";"
            << m.scrap_mm << ";"
            << m.goodLeftOver_Min_mm << ";"
            << m.goodLeftOver_Max_mm << ";"
            << m.externalCode << ";"
            << m.description << ";"
            << MaterialFamilyUtils::toString(m.family)
            << "\n";

        written++;
    }

    f.close();

    zInfo(QString("📦 exportCsv: %1 anyag kiírva ide: %2").arg(written).arg(path));
}


