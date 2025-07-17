#include "materialrepository.h"
#include <QFile>
#include <QTextStream>
#include <QUuid>

#include "materialrepository.h"
#include "materialmaster.h"
#include "materialtype.h"
#include "crosssectionshape.h"
#include "materialregistry.h"
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <common/filenamehelper.h>


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


    registry.setMaterials(loaded); // 🔧 Itt történik az anyagregisztráció


    return true;
}

QVector<MaterialMaster> MaterialRepository::loadFromCSV_private(const QString& filePath) {
    QVector<MaterialMaster> result;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Nem sikerült megnyitni a CSV fájlt: %s", qUtf8Printable(filePath));
        return result;
    }

    QTextStream in(&file);
    bool isFirstLine = true;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (isFirstLine) { isFirstLine = false; continue; }

        const QStringList columns = line.split(';');
        if (columns.size() < 8) continue;

        MaterialMaster m;  // automatikusan generált QUuid, shape és type default

        m.name = columns[0];                      // 📛 Név
        m.barcode = columns[1];                   // 📦 Barcode közvetlenül a CSV-ből
        m.stockLength_mm = columns[2].toDouble(); // 📏 Szálhossz
        m.defaultMachineId = columns[6];          // ⚙️ Gépid
        m.type = MaterialType::fromString(columns[7]); // 🧪 Típus

        // 🔍 Forma felismerés
        m.shape = CrossSectionShape::fromString(columns[5]);

        // 📐 Dimenziók a forma alapján
        if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Rectangular)) {
            m.size_mm = QSizeF(columns[3].toDouble(), columns[4].toDouble()); // width, height
        }
        else if (m.shape == CrossSectionShape(CrossSectionShape::Shape::Round)) {
            m.diameter_mm = columns[3].toDouble(); // diameter
        }
        else {
            m.size_mm = QSizeF();
            m.diameter_mm = 0.0;
        }

        result.append(m);
    }

    file.close();
    return result;
}

