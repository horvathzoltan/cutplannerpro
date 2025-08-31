#pragma once

#include <QString>
#include "../registries/cuttingmachineregistry.h"
#include "common/csvimporter.h"

/*
✂️ CuttingMachineRepository – Three Phase Import

CSV fájlok:
- cuttingmachines.csv → gépdefiníciók
- cuttingmachine_materials.csv → kompatibilis anyagtípusok

Fázisok:
1️⃣ Convert → CuttingMachineRow, CuttingMachineMaterialRow
2️⃣ Build → CuttingMachine objektum, MaterialType példány
3️⃣ Assemble → CuttingMachineRegistry::registerMachine
*/

class CuttingMachineRepository {
public:
    static bool loadFromCsv(CuttingMachineRegistry& registry);

private:
    struct CuttingMachineRow {
        QString name;
        QString barcode;
        QString location;
        QString kerf;
        QString stellerMaxLength;
        QString stellerCompensation;
        QString storageBarcode;
        QString comment;
    };

    struct CuttingMachineMaterialTypeRow {
        QString machineBarcode;
        QString materialTypeStr;
    };

    // --- Stage 1: Convert ---
    static std::optional<CuttingMachineRow> convertRowToMachineRow(const QVector<QString>& parts, CsvReader::RowContext& ctx);
    static std::optional<CuttingMachineMaterialTypeRow> convertRowToMaterialRow(const QVector<QString>& parts, CsvReader::RowContext& ctx);

    // --- Stage 2: Build ---
    static std::optional<CuttingMachine> buildMachineFromRow(const CuttingMachineRow& row, CsvReader::RowContext& ctx);
    static std::optional<MaterialType> buildMaterialTypeFromRow(const CuttingMachineMaterialTypeRow& row, CsvReader::RowContext& ctx);

    // --- Stage 3: Load & Assemble ---
    static QVector<CuttingMachineRow> loadMachineRows(const QString& filepath);
    static QVector<CuttingMachineMaterialTypeRow> loadMaterialTypeRows(const QString& filepath);
    static void addMaterialTypeToMachine(CuttingMachine* machine, const MaterialType& material);
};
