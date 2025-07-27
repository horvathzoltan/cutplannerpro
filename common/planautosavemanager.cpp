// #include "planautosavemanager.h"
// #include <QDateTime>
// #include <QDir>
// #include <QFileInfo>
// #include "filenamehelper.h"
// #include "../model/repositories/cuttingrequestrepository.h"
// #include "../model/registries/cuttingrequestregistry.h"

// PlanAutosaveManager::PlanAutosaveManager()
//     : settings(FileNameHelper::instance().getSettingsFilePath(), QSettings::IniFormat)
// {
//     currentPath = settings.value("LastPlanPath").toString();
// }

// QString PlanAutosaveManager::getCurrentPath() const {
//     return currentPath;
// }

// bool PlanAutosaveManager::hasActivePath() const {
//     return !currentPath.isEmpty() && QFile::exists(currentPath);
// }

// void PlanAutosaveManager::updatePlan(const QString& path) {
//     currentPath = path;
//     settings.setValue("LastPlanPath", path);
// }

// bool PlanAutosaveManager::loadLastPlan() {
//     if (!hasActivePath())
//         return false;

//     return CuttingRequestRepository::loadFromFile(CuttingRequestRegistry::instance(), currentPath);
// }

// void PlanAutosaveManager::saveCurrentPlan() {
//     if (!hasActivePath()) {
//         currentPath = generateAutosavePath();
//         settings.setValue("LastPlanPath", currentPath);
//     }

//     CuttingRequestRepository::saveToFile(CuttingRequestRegistry::instance(), currentPath);
// }

// QString PlanAutosaveManager::generateAutosavePath() const {
//     const QDateTime now = QDateTime::currentDateTime();
//     const QString timestamp = now.toString("yyyy_MM_dd_HH_mm_ss"); // pontosabb időbélyeg

//     const QString planName = "cutting_plan"; // 🔧 később akár paraméterezhető
//     const QString fileName = QString("%1_autosave_%2.csv").arg(planName, timestamp);

//     const QString folder = FileNameHelper::instance().getAutosaveFolder(); // 📁 már létrehozza, ha kell
//     return QDir(folder).filePath(fileName);
// }

