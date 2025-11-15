#include "startupmanager.h"
#include "materials/repository/material_repository.h"
#include "materials/registry/material_registry.h"
#include "materials/model/material_master.h"
#include "../logger.h"
#include "../../model/registries/cuttingplanrequestregistry.h"
//#include "model/stockentry.h"

#include "materials/repository/material_group_repository.h"
#include "../../model/registries/stockregistry.h"
#include "../../model/repositories/cuttingrequestrepository.h"
#include "../../model/repositories/leftoverstockrepository.h"
#include "../../model/repositories/stockrepository.h"
#include "../../model/repositories/storagerepository.h"

#include <QSet>

#include "../../model/repositories/cuttingmachinerepository.h"
#include "../../model/registries/cuttingmachineregistry.h"
#include "../eventlogger.h"
#include "../filenamehelper.h"

#include "../color/namedcolor.h"

StartupStatus StartupManager::runStartupSequence() {
    StartupStatus ralColorStatus = initRalColors();
    if (!ralColorStatus.isSuccess())
        return ralColorStatus;

    StartupStatus materialStatus = initMaterialRegistry();
    if (!materialStatus.isSuccess())
        return materialStatus;

    StartupStatus storageStatus = initStorageRegistry();
    if (!storageStatus.isSuccess())
        return storageStatus;

    StartupStatus groupStatus = initMaterialGroupRegistry();
    if (!groupStatus.isSuccess())
        return groupStatus;

    StartupStatus stockStatus = initStockRegistry();
    if (!stockStatus.isSuccess())
        return stockStatus;

    StartupStatus reusableStockStatus = initReusableStockRegistry();
    if (!reusableStockStatus.isSuccess())
        return reusableStockStatus;

    //CuttingRequestRepository::tryLoadFromSettings(CuttingRequestRegistry::instance());
    StartupStatus cuttingReqStatus = initCuttingRequestRegistry();
    if (!cuttingReqStatus.isSuccess())
        return cuttingReqStatus;

    StartupStatus cuttingMachineStatus = initCuttingMachineRegistry();
    if (!cuttingMachineStatus.isSuccess())
        return cuttingMachineStatus;

    StartupStatus finalStatus = StartupStatus::success();
    finalStatus.addWarnings(ralColorStatus.warnings());

    finalStatus.addWarnings(materialStatus.warnings());
    finalStatus.addWarnings(groupStatus.warnings());
    finalStatus.addWarnings(stockStatus.warnings());
    finalStatus.addWarnings(reusableStockStatus.warnings());
    finalStatus.addWarnings(cuttingReqStatus.warnings());
    finalStatus.addWarnings(cuttingMachineStatus.warnings());

    EventLogger::instance().zEvent(QString("🌱 Init összefoglaló: %1 anyag, %2 gép, %3 stock, %4 leftover")
                                       .arg(MaterialRegistry::instance().readAll().size())
                                       .arg(CuttingMachineRegistry::instance().readAll().size())
                                       .arg(StockRegistry::instance().readAll().size())
                                       .arg(LeftoverStockRegistry::instance().readAll().size()));

    return finalStatus;
}

StartupStatus StartupManager::initMaterialRegistry() {
    bool loaded = MaterialRepository::loadFromCSV(MaterialRegistry::instance());
    if (!loaded){
        EventLogger::instance().zEvent("❌ Nem sikerült betölteni az anyagtörzset");

        return StartupStatus::failure("❌ Nem sikerült betölteni az anyagtörzset a CSV fájlból.");
    }

    const auto& all = MaterialRegistry::instance().readAll();

    if (!hasMinimumMaterials(2)){
        EventLogger::instance().zEvent("❌ túl kevés adat az anyagtörzsben");

        return StartupStatus::failure(
            QString("⚠️ Túl kevés anyag található a törzsben (%1 db). Legalább 2 szükséges.")
                .arg(all.size()));
    }

    StartupStatus status = StartupStatus::success();

    // 🔎 Validáció: csoportban szereplő ismeretlen anyagok
    QSet<QUuid> knownMaterials;
    for (const auto& mat : all)
        knownMaterials.insert(mat.id);

    const auto& groupList = MaterialGroupRegistry::instance().readAll();
    QStringList invalidGroups;

    for (const auto& group : groupList) {
        for (const auto& mid : group.materialIds) {
            if (!knownMaterials.contains(mid)) {
                invalidGroups << group.name;
                break;
            }
        }
    }

    if (!invalidGroups.isEmpty()) {
        EventLogger::instance().zEvent("⚠️ az anyagcsoport adatai hibás elemeket tartalmaznak.");
        status.addWarning(
            QString("⚠️ %1 csoport olyan anyagot tartalmaz, ami nincs a törzsben.\nEllenőrizd a groups.csv fájlt: %2")
                .arg(invalidGroups.size())
                .arg(invalidGroups.join(", "))
            );
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(status.isSuccess(),"anyagtörzs init"));
    return status;
}

StartupStatus StartupManager::initStorageRegistry() {
    bool loaded = StorageRepository::loadFromCSV(StorageRegistry::instance());
    if (!loaded){
        EventLogger::instance().zEvent("❌ Nem sikerült betölteni a raktár törzset");

        return StartupStatus::failure("❌ Nem sikerült betölteni a storage.csv fájlból a raktár törzset.");
    }

    const auto& entries = StorageRegistry::instance().readAll();
    if (entries.isEmpty()){
        EventLogger::instance().zEvent("❌ nincs adat a raktár törzsben");

        return StartupStatus::failure("⚠️ A raktár törzs üres. Legalább 1 raktári bejegyzés szükséges.");
    }

    StartupStatus status = StartupStatus::success();

    // 🔎 Validáció: parentId mezők csak létező bejegyzésre mutassanak
    QSet<QUuid> knownIds;
    for (const auto& entry : entries)
        knownIds.insert(entry.id);

    QStringList invalidParentRefs;
    for (const auto& entry : entries) {
        if (!entry.parentId.isNull() && !knownIds.contains(entry.parentId)) {
            invalidParentRefs << entry.name;
        }
    }

    if (!invalidParentRefs.isEmpty()) {
        EventLogger::instance().zEvent("❌ a raktár törzs adatai hibás elemeket tartalmaznak.");
        status.addWarning(
            QString("⚠️ %1 storage elem nem létező szülőre hivatkozik.\nÉrintett bejegyzések: %2")
                .arg(invalidParentRefs.size())
                .arg(invalidParentRefs.join(", "))
            );
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(status.isSuccess(),"raktár törzs init"));
    return status;
}


StartupStatus StartupManager::initStockRegistry() {
    bool loaded = StockRepository::loadFromCSV(StockRegistry::instance());
    if (!loaded){
        EventLogger::instance().zEvent("❌ Nem sikerült betölteni a készletet");
        return StartupStatus::failure("❌ Nem sikerült betölteni a készletet a CSV fájlból).");
    }
    const auto& all = MaterialRegistry::instance().readAll();

    if (all.isEmpty()){
        EventLogger::instance().zEvent("❌ nincs adat a készletben");

        return StartupStatus::failure("⚠️ A készlet üres. Legalább 1 tétel szükséges a működéshez.");
    }

    StartupStatus status = StartupStatus::success();

    // 🔎 Validáció: minden stock-entry ismert anyagra hivatkozzon
    QSet<QUuid> knownMaterials;
    for (const auto& mat : all)
        knownMaterials.insert(mat.id);

    QStringList invalidStockItems;

    for (const auto& entry : all) {
        if (!knownMaterials.contains(entry.id)) {
            invalidStockItems << entry.id.toString();
        }
    }

    if (!invalidStockItems.isEmpty()) {
        EventLogger::instance().zEvent("❌ a készlet adatai hibás elemeket tartalmaznak.");
        status.addWarning(
            QString("⚠️ %1 készletelem nem létező anyagra hivatkozik.\nEllenőrizd a stock.csv fájlt.\nÉrintett azonosítók:\n%2")
                .arg(invalidStockItems.size())
                .arg(invalidStockItems.join(", "))
            );
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(status.isSuccess(),"készlet init"));
    return status;
}


StartupStatus StartupManager::initMaterialGroupRegistry() {
    bool loaded = MaterialGroupRepository::loadFromCsv(MaterialGroupRegistry::instance());
    if (!loaded){
        EventLogger::instance().zEvent("❌ Nem sikerült betölteni az anyagcsoportokat");

        return StartupStatus::failure("❌ Nem sikerült betölteni az anyagcsoportokat a groups.csv fájlból.");
    }

    int count = MaterialGroupRegistry::instance().readAll().size();
    if (count == 0){
        EventLogger::instance().zEvent("❌ nincs adat az anyagcsoportokban");

        return StartupStatus::failure("⚠️ Nem található egyetlen anyagcsoport sem. Lehet, hogy üres vagy hibás a fájl.");
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(true,"anyagcsoport init"));
    return StartupStatus::success();
}

bool StartupManager::hasMinimumMaterials(int minCount) {
    return MaterialRegistry::instance().readAll().size() >= minCount;
}

StartupStatus StartupManager::initReusableStockRegistry() {
    bool loaded = LeftoverStockRepository::loadFromCSV(LeftoverStockRegistry::instance());
    if (!loaded){
        EventLogger::instance().zEvent("❌ Nem sikerült betölteni a maradék készletet");
        return StartupStatus::failure("❌ Nem sikerült betölteni a maradék készletet a leftovers.csv fájlból.");
    }


    const auto& all = LeftoverStockRegistry::instance().readAll();

    if (all.isEmpty()){
        EventLogger::instance().zEvent("❌ nincs adat a maradék készletben");

        return StartupStatus::failure("⚠️ A maradék készlet üres. Legalább 1 tétel szükséges a működéshez.");
    }

    StartupStatus status = StartupStatus::success();

    // 🔎 Validáció: minden reusable-stock-entry ismert anyagra hivatkozzon
    QSet<QUuid> knownMaterials;
    for (const auto& mat : MaterialRegistry::instance().readAll())
        knownMaterials.insert(mat.id);

    QStringList invalidReusableStockItems;

    for (const auto& entry : all) {
        if (!knownMaterials.contains(entry.materialId)) {
            invalidReusableStockItems << entry.barcode;
        }
    }

    if (!invalidReusableStockItems.isEmpty()) {
        EventLogger::instance().zEvent("❌ a maradék készlet adatai hibás elemeket tartalmaznak.");

        status.addWarning(
            QString("⚠️ %1 maradék készletelem nem létező anyagra hivatkozik.Ellenőrizd a leftovers.csv fájlt.Érintett vonalkódok:%2")
                .arg(invalidReusableStockItems.size())
                .arg(invalidReusableStockItems.join(", "))
            );
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(status.isSuccess(),"maradék készlet init"));
    return status;
}

StartupStatus StartupManager::initCuttingRequestRegistry() {
    auto result = CuttingRequestRepository::tryLoadFromSettings(CuttingPlanRequestRegistry::instance());
    //if (!loaded)
    //    return StartupStatus::failure("❌ Nem sikerült betölteni a vágási igényeket a beállított vágási terv fájlból.");

    switch (result) {
    case CuttingPlanLoadResult::NoFileConfigured:
        zInfo("ℹ️ Nincs vágási terv konfigurálva — ez nem hiba.");
        return StartupStatus::success();

    case CuttingPlanLoadResult::FileMissing:
        EventLogger::instance().zEvent("❌ vágási terv fájl nem található");
        return StartupStatus::failure("❌ A beállított vágási terv fájl nem található.");

    case CuttingPlanLoadResult::LoadError:
        EventLogger::instance().zEvent("❌ vágási terv fájl nem olvasható");
        return StartupStatus::failure("❌ Nem sikerült betölteni a vágási igényeket — fájl hibás vagy olvashatatlan.");

    case CuttingPlanLoadResult::Success:
        break;
    }

    const auto& all = CuttingPlanRequestRegistry::instance().readAll();

    // 💡 Check: file might be valid but intentionally empty (just header)
    if (all.isEmpty() && CuttingRequestRepository::wasLastFileEffectivelyEmpty()) {        
        zInfo("ℹ️ Nincsenek vágási igények — új terv indítása vagy adat még nem érkezett");
        return StartupStatus::success();
    }

    if (all.isEmpty()){
        EventLogger::instance().zEvent("❌ nincs adat az vágási igényekben");

        return StartupStatus::failure("⚠️ A vágási igény lista üres. Legalább 1 tétel szükséges.");
    }

    StartupStatus status = StartupStatus::success();

    // 🔎 Validáció: minden request ismert anyagra hivatkozzon
    QSet<QUuid> knownMaterials;
    for (const auto& mat : MaterialRegistry::instance().readAll())
        knownMaterials.insert(mat.id);

    QStringList invalidRequests;
    for (const Cutting::Plan::Request &req : all) {
        if (!knownMaterials.contains(req.materialId)) {
            QString desc = req.toString();
            invalidRequests << desc; // vagy req.id.toString() ha azonosító kell
        }
    }

    if (!invalidRequests.isEmpty()) {
        EventLogger::instance().zEvent("⚠️ vágási terv adatai hibás elemeket tartalmaznak.");
        status.addWarning(            
            QString("⚠️ %1 vágási igény nem létező anyagra hivatkozik.\nEllenőrizd a cuttingrequests.csv fájlt.\nÉrintett azonosítók:\n%2")
                .arg(invalidRequests.size())
                .arg(invalidRequests.join(", "))
            );
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(status.isSuccess(),"vágási terv init"));
    return status;
}

StartupStatus StartupManager::initCuttingMachineRegistry() {
    bool loaded = CuttingMachineRepository::loadFromCsv(CuttingMachineRegistry::instance());
    if (!loaded){
        EventLogger::instance().zEvent("❌ Nem sikerült betölteni a vágógépeket");
        return StartupStatus::failure("❌ Nem sikerült betölteni a vágógépeket a cuttingmachines.csv fájlból.");
    }

    const auto& machines = CuttingMachineRegistry::instance().readAll();
    if (machines.isEmpty()){
        EventLogger::instance().zEvent("❌ nincs adat a vágőgépekben");

        return StartupStatus::failure("⚠️ A vágógép törzs üres. Legalább 1 gép szükséges a működéshez.");
    }

    StartupStatus status = StartupStatus::success();

    // 🔎 Validáció: minden géphez legyen legalább 1 kompatibilis anyagtípus
    QStringList machinesWithoutMaterials;
    for (const auto& machine : machines) {
        if (machine.compatibleMaterials.isEmpty())
            machinesWithoutMaterials << machine.name;
    }

    if (!machinesWithoutMaterials.isEmpty()) {
        EventLogger::instance().zEvent("❌ a vágógépek adatai hibás elemeket tartalmaznak.");

        status.addWarning(
            QString("⚠️ %1 vágógéphez nincs megadva kompatibilis anyagtípus.\nÉrintett gépek: %2")
                .arg(machinesWithoutMaterials.size())
                .arg(machinesWithoutMaterials.join(", ")));
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(status.isSuccess(),"vágógépek init"));
    return status;
}

StartupStatus StartupManager::initRalColors()
{
    auto fnh = FileNameHelper::instance();

    QList<RalSource> ralSources = {
        { RalSystem::Classic, fnh.getRalClassicCsvFile() },
        { RalSystem::Design,  fnh.getRalDesignCsvFile() },
        { RalSystem::Plastic1,  fnh.getRalPlastic1CsvFile() },
        { RalSystem::Plastic2,  fnh.getRalPlastic2CsvFile() }
    };

    bool ok = NamedColor::initRalColors(ralSources);

    StartupStatus status = StartupStatus::success();

    if (!ok) {
        EventLogger::instance().zEvent("❌ a RAL színek adatai hibás elemeket tartalmaznak.");

        status.addWarning(
            QString("⚠️ Nem sikerült a RAL színeket initelni.")
            );
    }

    EventLogger::instance().zEvent(StatusHelper::getMessage(status.isSuccess(),"RAL szinek init"));
    return status;
}

