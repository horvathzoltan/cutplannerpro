#include "startupmanager.h"
#include "../../model/repositories/materialrepository.h"
#include "../../model/registries/materialregistry.h"
#include "../../model/materialmaster.h"
#include "common/logger.h"
#include "model/registries/cuttingplanrequestregistry.h"
//#include "model/stockentry.h"

#include <model/repositories/materialgrouprepository.h>
#include <model/registries//stockregistry.h>
#include <model/repositories/cuttingrequestrepository.h>
#include <model/repositories/leftoverstockrepository.h>
#include <model/repositories/stockrepository.h>
#include <model/repositories/storagerepository.h>

#include <QSet>
//#include "ProfileCategory.h"

StartupStatus StartupManager::runStartupSequence() {      
    StartupStatus materialStatus = initMaterialRegistry();
    if (!materialStatus.ok)
        return materialStatus;

    StartupStatus storageStatus = initStorageRegistry();
    if (!storageStatus.ok)
        return storageStatus;

    StartupStatus groupStatus = initMaterialGroupRegistry(); // ✅ új név!
    if (!groupStatus.ok)
        return groupStatus;

    StartupStatus stockStatus = initStockRegistry(); // ✅ új név!
    if (!stockStatus.ok)
        return stockStatus;

    StartupStatus reusableStockStatus = initReusableStockRegistry();
    if (!reusableStockStatus.ok)
        return reusableStockStatus;

    //CuttingRequestRepository::tryLoadFromSettings(CuttingRequestRegistry::instance());
    StartupStatus cuttingReqStatus = initCuttingRequestRegistry();
    if (!cuttingReqStatus.ok)
        return cuttingReqStatus;

    StartupStatus finalStatus = StartupStatus::success();
    finalStatus.warnings += materialStatus.warnings;
    finalStatus.warnings += groupStatus.warnings;
    finalStatus.warnings += stockStatus.warnings;    
    finalStatus.warnings += reusableStockStatus.warnings;

    finalStatus.warnings += cuttingReqStatus.warnings;

    return finalStatus;
}

StartupStatus StartupManager::initMaterialRegistry() {
    bool loaded = MaterialRepository::loadFromCSV(MaterialRegistry::instance());
    if (!loaded)
        return StartupStatus::failure("❌ Nem sikerült betölteni az anyagtörzset a CSV fájlból.");

    const auto& all = MaterialRegistry::instance().readAll();

    if (!hasMinimumMaterials(2))
        return StartupStatus::failure(
            QString("⚠️ Túl kevés anyag található a törzsben (%1 db). Legalább 2 szükséges.")
                .arg(all.size()));

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
        status.addWarning(
            QString("⚠️ %1 csoport olyan anyagot tartalmaz, ami nincs a törzsben.\nEllenőrizd a groups.csv fájlt: %2")
                .arg(invalidGroups.size())
                .arg(invalidGroups.join(", "))
            );
    }

    return status;
}

StartupStatus StartupManager::initStorageRegistry() {
    bool loaded = StorageRepository::loadFromCSV(StorageRegistry::instance());
    if (!loaded)
        return StartupStatus::failure("❌ Nem sikerült betölteni a storage.csv fájlból a raktári törzset.");

    const auto& entries = StorageRegistry::instance().readAll();
    if (entries.isEmpty())
        return StartupStatus::failure("⚠️ A storage törzs üres. Legalább 1 raktári bejegyzés szükséges.");

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
        status.addWarning(
            QString("⚠️ %1 storage elem nem létező szülőre hivatkozik.\nÉrintett bejegyzések: %2")
                .arg(invalidParentRefs.size())
                .arg(invalidParentRefs.join(", "))
            );
    }

    return status;
}


StartupStatus StartupManager::initStockRegistry() {
    bool loaded = StockRepository::loadFromCSV(StockRegistry::instance());
    if (!loaded)
        return StartupStatus::failure("❌ Nem sikerült betölteni a készletet a CSV fájlból).");

    const auto& all = MaterialRegistry::instance().readAll();

    if (all.isEmpty())
        return StartupStatus::failure("⚠️ A készlet üres. Legalább 1 tétel szükséges a működéshez.");

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
        status.addWarning(
            QString("⚠️ %1 készletelem nem létező anyagra hivatkozik.\nEllenőrizd a stock.csv fájlt.\nÉrintett azonosítók:\n%2")
                .arg(invalidStockItems.size())
                .arg(invalidStockItems.join(", "))
            );
    }

    return status;
}


StartupStatus StartupManager::initMaterialGroupRegistry() {
    bool loaded = MaterialGroupRepository::loadFromCsv(MaterialGroupRegistry::instance());
    if (!loaded)
        return StartupStatus::failure("❌ Nem sikerült betölteni az anyagcsoportokat a groups.csv fájlból.");

    int count = MaterialGroupRegistry::instance().readAll().size();
    if (count == 0)
        return StartupStatus::failure("⚠️ Nem található egyetlen anyagcsoport sem. Lehet, hogy üres vagy hibás a fájl.");

    return StartupStatus::success();
}

bool StartupManager::hasMinimumMaterials(int minCount) {
    return MaterialRegistry::instance().readAll().size() >= minCount;
}

StartupStatus StartupManager::initReusableStockRegistry() {
    bool loaded = LeftoverStockRepository::loadFromCSV(LeftoverStockRegistry::instance());
    if (!loaded)
        return StartupStatus::failure("❌ Nem sikerült betölteni a maradék készletet a leftovers.csv fájlból.");

    const auto& all = LeftoverStockRegistry::instance().readAll();

    if (all.isEmpty())
        return StartupStatus::failure("⚠️ A maradék készlet üres. Legalább 1 tétel szükséges a működéshez.");

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
        status.addWarning(
            QString("⚠️ %1 maradék készletelem nem létező anyagra hivatkozik.Ellenőrizd a leftovers.csv fájlt.Érintett vonalkódok:%2")
                .arg(invalidReusableStockItems.size())
                .arg(invalidReusableStockItems.join(", "))
            );
    }

    return status;
}

StartupStatus StartupManager::initCuttingRequestRegistry() {
    bool loaded = CuttingRequestRepository::tryLoadFromSettings(CuttingPlanRequestRegistry::instance());
    if (!loaded)
        return StartupStatus::failure("❌ Nem sikerült betölteni a vágási igényeket a beállított vágási terv fájlból.");

    const auto& all = CuttingPlanRequestRegistry::instance().readAll();

    // 💡 Check: file might be valid but intentionally empty (just header)
    if (all.isEmpty() && CuttingRequestRepository::wasLastFileEffectivelyEmpty()) {
        zInfo("ℹ️ Nincsenek vágási igények — új terv indítása vagy adat még nem érkezett");
        return StartupStatus::success();
    }

    if (all.isEmpty())
        return StartupStatus::failure("⚠️ A vágási igény lista üres. Legalább 1 tétel szükséges.");

    StartupStatus status = StartupStatus::success();

    // 🔎 Validáció: minden request ismert anyagra hivatkozzon
    QSet<QUuid> knownMaterials;
    for (const auto& mat : MaterialRegistry::instance().readAll())
        knownMaterials.insert(mat.id);

    QStringList invalidRequests;
    for (const CuttingPlanRequest &req : all) {
        if (!knownMaterials.contains(req.materialId)) {
            QString desc = req.toString();
            invalidRequests << desc; // vagy req.id.toString() ha azonosító kell
        }
    }

    if (!invalidRequests.isEmpty()) {
        status.addWarning(
            QString("⚠️ %1 vágási igény nem létező anyagra hivatkozik.\nEllenőrizd a cuttingrequests.csv fájlt.\nÉrintett azonosítók:\n%2")
                .arg(invalidRequests.size())
                .arg(invalidRequests.join(", "))
            );
    }

    return status;
}


