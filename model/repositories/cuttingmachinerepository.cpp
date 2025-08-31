#include "cuttingmachinerepository.h"
#include "common/csvimporter.h"
#include "model/cutting/cuttingmachine.h"
#include "model/material/materialtype.h"
#include "common/filenamehelper.h"
#include "model/registries/cuttingmachineregistry.h"
#include <QDebug>
#include <model/registries/storageregistry.h>

// --- Stage 1: Convert ---

std::optional<CuttingMachineRepository::CuttingMachineRow>
CuttingMachineRepository::convertRowToMachineRow(const QVector<QString>& parts, CsvReader::RowContext& ctx) {
    if (parts.size() < 8) {
        qWarning() << "❌ Érvénytelen gépsor a sorban:" << ctx.lineIndex;
        return std::nullopt;
    }

    CuttingMachineRow row;
    row.name = parts[0].trimmed();
    row.barcode = parts[1].trimmed();
    row.location = parts[2].trimmed();
    row.kerf = parts[3].trimmed();
    row.stellerMaxLength = parts[4].trimmed();
    row.stellerCompensation = parts[5].trimmed();
    row.storageBarcode = parts[6].trimmed();
    row.comment = parts[7].trimmed();
    return row;
}

std::optional<CuttingMachineRepository::CuttingMachineMaterialTypeRow>
CuttingMachineRepository::convertRowToMaterialRow(const QVector<QString>& parts, CsvReader::RowContext& ctx) {
    if (parts.size() < 2) {
        qWarning() << "❌ Érvénytelen anyagsor a sorban:" << ctx.lineIndex;
        return std::nullopt;
    }

    CuttingMachineMaterialTypeRow row;
    row.machineBarcode = parts[0].trimmed();
    row.materialTypeStr = parts[1].trimmed();

    return row;
}

// --- Stage 2: Build ---

std::optional<CuttingMachine>
CuttingMachineRepository::buildMachineFromRow(const CuttingMachineRow& row, CsvReader::RowContext& ctx) {
    bool ok1, ok2, ok3;
    double kerf = row.kerf.toDouble(&ok1);
    double maxLen = row.stellerMaxLength.toDouble(&ok2);
    double comp = row.stellerCompensation.toDouble(&ok3);

    if (!ok1 || !ok2 || !ok3) {
        qWarning() << "❌ Hibás számformátum a sorban:" << ctx.lineIndex;
        return std::nullopt;
    }

    CuttingMachine machine;
    machine.id = QUuid::createUuid();
    machine.name = row.name;
    machine.barcode = row.barcode;
    machine.location = row.location;
    machine.kerf_mm = kerf;
    machine.stellerMaxLength_mm = maxLen;
    machine.stellerCompensation_mm = comp;
    machine.comment = row.comment;

    return std::make_optional(machine);
}

std::optional<MaterialType>
CuttingMachineRepository::buildMaterialTypeFromRow(const CuttingMachineMaterialTypeRow& row, CsvReader::RowContext& ctx) {
    if (row.materialTypeStr.isEmpty()) {
        qWarning() << "❌ Hiányzó anyagtípus a sorban:" << ctx.lineIndex;
        return std::nullopt;
    }

    MaterialType type = MaterialType::fromString(row.materialTypeStr);
    if (type.value == MaterialType::Type::Unknown) {
        qWarning() << "⚠️ Ismeretlen anyagtípus:" << row.materialTypeStr << "(gép barcode:" << row.machineBarcode << ")";
        return std::nullopt;
    }

    return std::make_optional(type);
}

// --- Stage 3: Load & Assemble ---

QVector<CuttingMachineRepository::CuttingMachineRow>
CuttingMachineRepository::loadMachineRows(const QString& filepath) {
    return CsvReader::readAndConvert<CuttingMachineRow>(filepath, convertRowToMachineRow);
}

QVector<CuttingMachineRepository::CuttingMachineMaterialTypeRow>
CuttingMachineRepository::loadMaterialTypeRows(const QString& filepath) {
    return CsvReader::readAndConvert<CuttingMachineMaterialTypeRow>(filepath, convertRowToMaterialRow);
}

void CuttingMachineRepository::addMaterialTypeToMachine(CuttingMachine* machine, const MaterialType& materialType) {
    if (!machine) return;
    machine->addMaterialType(materialType);
}

// --- Entry Point ---

bool CuttingMachineRepository::loadFromCsv(CuttingMachineRegistry& registry) {
    const auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString metaPath = helper.getCuttingMachineCsvFile();            // cuttingmachines.csv
    const QString compatPath = helper.getCuttingMachineMaterialsCsvFile(); // cuttingmachine_materials.csv

    const auto machineRows = loadMachineRows(metaPath);
    const auto materialTypeRows = loadMaterialTypeRows(compatPath);

    QMap<QString, CuttingMachine> machineMap;

    // 🔄 Gépek beolvasása és létrehozása
    for (int i = 0; i < machineRows.size(); ++i) {
        const auto& row = machineRows[i];

        CsvReader::RowContext ctx(i+1, metaPath);

        auto machineOpt = buildMachineFromRow(row, ctx);
        if (machineOpt.has_value()) {
            if (machineMap.contains(row.barcode)) {
                qWarning() << "⚠️ Duplikált gép barcode:" << row.barcode << "a sorban:" << i + 1;
            }

            CuttingMachine machine = machineOpt.value();

            // 🔍 Tároló keresése a géphez tartozó storageBarcode alapján
            const StorageEntry* storage = StorageRegistry::instance().findByBarcode(row.storageBarcode);
            if (storage) {
                // 📌 rootStorageId beállítása, hogy a gép tudja, hol tárol
                machine.rootStorageId = storage->id;
            } else {
                // ⚠️ Figyelmeztetés, ha a storageBarcode nem található
                qWarning() << "⚠️ Storage barcode nem található:" << row.storageBarcode << "a gépnél:" << row.name;
            }

            machineMap.insert(row.barcode, machine);
        }
    }

    // 🔗 Anyagtípusok hozzárendelése a gépekhez
    for (int i = 0; i < materialTypeRows.size(); ++i) {
        const auto& row = materialTypeRows[i];
        if (!machineMap.contains(row.machineBarcode)) {
            qWarning() << "⚠️ Ismeretlen gép barcode:" << row.machineBarcode << "a sorban:" << i + 1;
            continue;
        }

        CsvReader::RowContext ctx(i+1, compatPath);

        auto typeOpt = buildMaterialTypeFromRow(row, ctx);
        if (!typeOpt.has_value()) continue;

        CuttingMachine& machine = machineMap[row.machineBarcode];
        addMaterialTypeToMachine(&machine, typeOpt.value());
    }

    // 🧾 Gépek regisztrálása a rendszerbe
    for (auto it = machineMap.constBegin(); it != machineMap.constEnd(); ++it) {
        registry.registerData(it.value());
    }

    return true;
}



// bool CuttingMachineRepository::loadFromCsv(CuttingMachineRegistry& registry) {
//     const auto& helper = FileNameHelper::instance();
//     if (!helper.isInited()) return false;

//     const QString metaPath = helper.getCuttingMachineCsvFile();            // cuttingmachines.csv
//     const QString compatPath = helper.getCuttingMachineMaterialsCsvFile(); // cuttingmachine_materials.csv

//     const auto machineRows = loadMachineRows(metaPath);
//     const auto materialRows = loadMaterialRows(compatPath);

//     QMap<QString, CuttingMachine> machineMap;

//     for (int i = 0; i < machineRows.size(); ++i) {
//         const auto& row = machineRows[i];
//         auto machineOpt = buildMachineFromRow(row, i + 1);
//         if (machineOpt.has_value()) {
//             if (machineMap.contains(row.barcode)) {
//                 qWarning() << "⚠️ Duplikált gép barcode:" << row.barcode << "a sorban:" << i + 1;
//             }
//             machineMap.insert(row.barcode, machineOpt.value());
//         }
//     }

//     for (int i = 0; i < materialRows.size(); ++i) {
//         const auto& row = materialRows[i];
//         if (!machineMap.contains(row.machineBarcode)) {
//             qWarning() << "⚠️ Ismeretlen gép barcode:" << row.machineBarcode << "a sorban:" << i + 1;
//             continue;
//         }

//         auto typeOpt = buildMaterialTypeFromRow(row, i + 1);
//         if (!typeOpt.has_value()) continue;

//         CuttingMachine machine = machineMap[row.machineBarcode];
//         addMaterialToMachine(&machine, typeOpt.value());
//     }

//     for (auto it = machineMap.constBegin(); it != machineMap.constEnd(); ++it) {
//         registry.registerData(it.value());
//     }

//     return true;
// }
