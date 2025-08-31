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

bool MaterialRepository::loadFromCSV(MaterialRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    auto fn = helper.getMaterialCsvFile();
    if (fn.isEmpty()) {
        zWarning("Nem található a tesztadatok CSV fájlja.");
        return false;
    }

    const QVector<MaterialMaster> loaded = loadFromCSV_private(fn);
    if (loaded.isEmpty()) {
        zWarning("A materials.csv fájl üres vagy hibás formátumú.");
        return false;
    }

    registry.setData(loaded); // 🔧 Itt történik az anyagregisztráció
    return true;
}

QVector<MaterialMaster>
MaterialRepository::loadFromCSV_private(const QString& filePath) {
    return CsvReader::readAndConvert<MaterialMaster>(filePath, convertRowToMaterial, true);
}

std::optional<MaterialMaster>
MaterialRepository::convertRowToMaterial(const QVector<QString>& parts, CsvReader::RowContext& ctx) {
    const auto rowOpt = convertRowToMaterialRow(parts, ctx);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildMaterialFromRow(rowOpt.value(), ctx);
}

std::optional<MaterialRepository::MaterialRow>
MaterialRepository::convertRowToMaterialRow(const QVector<QString>& parts, CsvReader::RowContext& ctx) {
    if (parts.size() < 9) {
        QString msg = L("⚠️ Sor %1: kevés mező (legalább 9)").arg(ctx.lineIndex);
        zWarning(msg);
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
    row.colorStr = parts[8].trimmed(); // csak ha van legalább 9 mező

    const QString lengthStr = parts[2].trimmed();
    bool okLength = false;
    row.stockLength = lengthStr.toDouble(&okLength);

    if (row.barcode.isEmpty() || !okLength || row.stockLength <= 0) {
        QString msg = L("⚠️ Sor %1: érvénytelen barcode vagy hossz").arg(ctx.lineIndex);
        zWarning(msg);
        return std::nullopt;
    }

    return row;
}

std::optional<MaterialMaster>
MaterialRepository::buildMaterialFromRow(const MaterialRow& row, CsvReader::RowContext& ctx) {
    MaterialMaster m;
    m.id = QUuid::createUuid();
    m.name = row.name;
    m.barcode = row.barcode;
    m.stockLength_mm = row.stockLength;
    m.defaultMachineId = row.machineId;
    m.shape = CrossSectionShape::fromString(row.shapeStr);
    m.type = MaterialType::fromString(row.typeStr);

    if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Rectangular)) {
        bool okW = false, okH = false;
        double w = row.dim1.toDouble(&okW);
        double h = row.dim2.toDouble(&okH);
        if (!okW || !okH || w <= 0 || h <= 0) {
            qWarning() << QString("⚠️ Sor %1: érvénytelen szélesség/magasság").arg(ctx.lineIndex);
            //return std::nullopt;
        }
        m.size_mm = QSizeF(w, h);
    }
    else if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Round)) {
        bool okD = false;
        double d = row.dim1.toDouble(&okD);
        if (!okD || d <= 0) {
            qWarning() << QString("⚠️ Sor %1: érvénytelen átmérő").arg(ctx.lineIndex);
            //return std::nullopt;
        }
        m.diameter_mm = d;
    }

    // 🎨 Szín hozzárendelés – RAL, HEX vagy üres
    if (!row.colorStr.isEmpty()) {
        m.color = NamedColor(row.colorStr);
        if (!m.color.isValid()) {
            QString msg =  QStringLiteral("⚠️ Sor %1: ismeretlen színformátum: %2").arg(ctx.lineIndex).arg( row.colorStr);
            zWarning(msg);
        }
    } else {
        m.color = NamedColor(); // nincs festve
    }

    return m;
}


