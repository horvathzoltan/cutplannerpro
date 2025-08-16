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
#include <QDebug>
#include <common/csvimporter.h>

bool MaterialRepository::loadFromCSV(MaterialRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    auto fn = helper.getMaterialCsvFile();
    if (fn.isEmpty()) {
        qWarning("Nem található a tesztadatok CSV fájlja.");
        return false;
    }

    const QVector<MaterialMaster> loaded = loadFromCSV_private(fn);
    if (loaded.isEmpty()) {
        qWarning("A materials.csv fájl üres vagy hibás formátumú.");
        return false;
    }


    registry.setData(loaded); // 🔧 Itt történik az anyagregisztráció

    /*for (const auto& m : std::as_const(loaded))
        registry.registerData(m);*/

    return true;
}

// QVector<MaterialMaster> MaterialRepository::loadFromCSV_private(const QString& filePath) {
//     QVector<MaterialMaster> result;
//     QFile file(filePath);

//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning() << "❌ Nem sikerült megnyitni a CSV fájlt:" << filePath;
//         return result;
//     }

//     QTextStream in(&file);
//     in.setEncoding(QStringConverter::Utf8);

//     const QList<QVector<QString>> rows = FileHelper::parseCSV(&in, ';');
//     if (rows.size() <= 1) {
//         qWarning() << "⚠️ Üres vagy csak fejlécet tartalmaz a fájl.";
//         return result;
//     }

//     for (int i = 1; i < rows.size(); ++i) {
//         const auto maybeMaterial = convertRowToMaterial(rows[i], i + 1);
//         if (maybeMaterial.has_value())
//             result.append(maybeMaterial.value());
//     }

//     return result;
// }

QVector<MaterialMaster>
MaterialRepository::loadFromCSV_private(const QString& filePath) {
    // QFile file(filePath);
    // if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    //     qWarning() << "❌ Nem sikerült megnyitni a CSV fájlt:" << filePath;
    //     return {};
    // }

    // QTextStream in(&file);
    // in.setEncoding(QStringConverter::Utf8);

    // const auto rows = FileHelper::parseCSV(&in, ';');
    // return CsvImporter::processCsvRows<MaterialMaster>(rows, convertRowToMaterial);

    return CsvReader::readAndConvert<MaterialMaster>(filePath, convertRowToMaterial, true);
}

std::optional<MaterialMaster>
MaterialRepository::convertRowToMaterial(const QVector<QString>& parts, int lineIndex) {
    const auto rowOpt = convertRowToMaterialRow(parts, lineIndex);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildMaterialFromRow(rowOpt.value(), lineIndex);
}


std::optional<MaterialRepository::MaterialRow>
MaterialRepository::convertRowToMaterialRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 8) {
        qWarning() << QString("⚠️ Sor %1: kevés mező (legalább 8)").arg(lineIndex);
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

    const QString lengthStr = parts[2].trimmed();
    bool okLength = false;
    row.stockLength = lengthStr.toDouble(&okLength);

    if (row.barcode.isEmpty() || !okLength || row.stockLength <= 0) {
        qWarning() << QString("⚠️ Sor %1: érvénytelen barcode vagy hossz").arg(lineIndex);
        return std::nullopt;
    }

    return row;
}

std::optional<MaterialMaster>
MaterialRepository::buildMaterialFromRow(const MaterialRow& row, int lineIndex) {
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
            qWarning() << QString("⚠️ Sor %1: érvénytelen szélesség/magasság").arg(lineIndex);
            return std::nullopt;
        }
        m.size_mm = QSizeF(w, h);
    }
    else if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Round)) {
        bool okD = false;
        double d = row.dim1.toDouble(&okD);
        if (!okD || d <= 0) {
            qWarning() << QString("⚠️ Sor %1: érvénytelen átmérő").arg(lineIndex);
            return std::nullopt;
        }
        m.diameter_mm = d;
    }

    return m;
}



// QVector<MaterialMaster> MaterialRepository::loadFromCSV_private(const QString& filePath) {
//     QVector<MaterialMaster> result;
//     QFile file(filePath);

//     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         qWarning("Nem sikerült megnyitni a CSV fájlt: %s", qUtf8Printable(filePath));
//         return result;
//     }

//     QTextStream in(&file);
//     bool isFirstLine = true;

//     while (!in.atEnd()) {
//         QString line = in.readLine().trimmed();
//         if (line.isEmpty()) continue;
//         if (isFirstLine) { isFirstLine = false; continue; }

//         const QStringList columns = line.split(';');
//         if (columns.size() < 8) continue;

//         MaterialMaster m;  // automatikusan generált QUuid, shape és type default

//         m.id = QUuid::createUuid(); // 🆔 Automatikus azonosító generálás

//         m.name = columns[0];                      // 📛 Név
//         m.barcode = columns[1];                   // 📦 Barcode közvetlenül a CSV-ből
//         m.stockLength_mm = columns[2].toDouble(); // 📏 Szálhossz
//         m.defaultMachineId = columns[6];          // ⚙️ Gépid
//         m.type = MaterialType::fromString(columns[7]); // 🧪 Típus

//         //m.category = CategoryUtils::categoryFromString(columns[8]);

//         // 🔍 Forma felismerés
//         m.shape = CrossSectionShape::fromString(columns[5]);

//         // 📐 Dimenziók a forma alapján
//         if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Rectangular)) {
//             m.size_mm = QSizeF(columns[3].toDouble(), columns[4].toDouble()); // width, height
//         }
//         else if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Round)) {
//             m.diameter_mm = columns[3].toDouble(); // diameter
//         }
//         else {
//             m.size_mm = QSizeF();
//             m.diameter_mm = 0.0;
//         }

//         result.append(m);
//     }

//     file.close();
//     return result;
// }

