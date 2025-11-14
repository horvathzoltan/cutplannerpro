#include "materialrepository.h"
#include <QFile>
#include <QTextStream>
#include <QUuid>

//#include "common/categoryutils.h"
#include "materialrepository.h"
#include "../material/materialmaster.h"
#include "../material/materialtype.h"
#include "../crosssectionshape.h"
#include "../registries/materialregistry.h"
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <common/filehelper.h>
#include <common/filenamehelper.h>
#include <common/csvimporter.h>
#include <common/color/namedcolor.h>
#include "../material/cuttingmode.h"
#include "../material/paintingmode.h"
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

    return m;
}


