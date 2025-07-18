#include "startupmanager.h"
#include "../../model/materialrepository.h"
#include "../../model/materialregistry.h"
#include "../../model/materialmaster.h"
//#include "ProfileCategory.h"

StartupStatus StartupManager::runStartupSequence() {
    StartupStatus materialStatus = initMaterialRegistry();
    if (!materialStatus.ok)
        return materialStatus;

    // Jövőbeli bővítéshez: initMachineRegistry(), initConfig(), stb.

    return StartupStatus::success();
}

StartupStatus StartupManager::initMaterialRegistry() {
    bool loaded = MaterialRepository::loadFromCSV(MaterialRegistry::instance());
    if (!loaded)
        return StartupStatus::failure("❌ Nem sikerült betölteni az anyagtörzset a CSV fájlból.");

    const auto& all = MaterialRegistry::instance().all();
    if (!hasMinimumMaterials(2))
        return StartupStatus::failure(
            QString("⚠️ Túl kevés anyag található a törzsben (%1 db). Legalább 2 szükséges.")
                .arg(all.size()));

    int unknowns = std::count_if(all.begin(), all.end(), [](const MaterialMaster& m) {
        return m.category == ProfileCategory::Unknown;
    });

    StartupStatus status = StartupStatus::success();
    if (unknowns > 0) {
        status.addWarning(
            QString("⚠️ %1 anyag ismeretlen kategóriájú (Unknown). Ellenőrizd a CSV fájlt.")
                .arg(unknowns));
    }

    return status;
}

bool StartupManager::hasMinimumMaterials(int minCount) {
    return MaterialRegistry::instance().all().size() >= minCount;
}
