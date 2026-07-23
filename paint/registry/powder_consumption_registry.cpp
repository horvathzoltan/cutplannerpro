// #include "powder_consumption_registry.h"

// #include <QFile>
// #include <QTextStream>

// bool PowderConsumptionRegistry::loadFromCsv(const QString& path)
// {
//     QFile f(path);
//     if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
//         zError("PowderConsumptionRegistry: nem tudom megnyitni: " + path);
//         return false;
//     }

//     QTextStream in(&f);
//     int count = 0;

//     while (!in.atEnd())
//     {
//         QString line = in.readLine().trimmed();
//         if (line.isEmpty())
//             continue;
//         if (line.startsWith("#"))
//             continue;

//         // CSV: typeCode;subtypeCode;length;weight;colorMultiplier;surfaceMultiplier
//         auto parts = line.split(';');
//         if (parts.size() < 6) {
//             zWarn("PowderConsumptionRegistry: hibás sor: " + line);
//             continue;
//         }

//         QString typeCode    = parts[0].trimmed();
//         QString subtypeCode = parts[1].trimmed();

//         auto typeId = ProductTypeRegistry::instance().findIdByCode(typeCode);
//         auto subtypeId = ProductSubtypeRegistry::instance().findIdByCode(subtypeCode);

//         if (typeId.isNull() || subtypeId.isNull()) {
//             zWarn("PowderConsumptionRegistry: ismeretlen type/subtype: " + line);
//             continue;
//         }

//         PowderConsumptionModel m;
//         m.productTypeId    = typeId;
//         m.productSubtypeId = subtypeId;
//         m.length           = parts[2].toDouble();
//         m.weight           = parts[3].toDouble();
//         m.colorMultiplier  = parts[4].toDouble();
//         m.surfaceMultiplier= parts[5].toDouble();

//         QString key = makeKey(typeId, subtypeId);
//         map[key] = m;
//         count++;
//     }

//     zInfo(QString("PowderConsumptionRegistry: %1 sor betöltve").arg(count));
//     return true;
// }
