#include "filenamehelper.h"
//#include "settingsmanager.h"
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

FileNameHelper::FileNameHelper() {
    _isInited = init(__FILE__);
}

bool FileNameHelper::init(const char* file) {
#if defined(STRINGIFY_H) && defined(STRING) && defined(SOURCE_PATH)
    Q_UNUSED(file);
    _projectPath = STRING(SOURCE_PATH);
    return true;
#else
    if (file) {
        QString path(file);
        if (path.endsWith("/helpers/filenamehelper.cpp") || path.endsWith("/common/filenamehelper.cpp")) {
            QDir dir = QFileInfo(path).absoluteDir();
            dir.cdUp(); // 🔙 projekt gyökér
            auto applicationPath = dir.absolutePath();
            _projectPath = QDir(applicationPath).filePath("testdata");
            return true;
        }
    }
    // ❌ Hiba logolható itt, ha szükséges


    auto applicationPath = QCoreApplication::applicationDirPath();

    _projectPath = QDir(applicationPath).filePath("testdata");

    QDir fallbackTestDir(applicationPath);
    if (!fallbackTestDir.exists("testdata")) {
        qWarning("Fallback aktiválva, de nincs 'testdata' mappa a bináris mappában: %s", qUtf8Printable(applicationPath));

            qWarning("Aktuális bináris mappa: %s", qUtf8Printable(applicationPath));
        return false;
    }

    // ✅ fallback sikeres és valid
    qDebug("✅ Fallback init sikeres, használjuk: %s", qUtf8Printable(applicationPath));
    return true;
#endif
}

FileNameHelper& FileNameHelper::instance(const char* file) {
    static FileNameHelper helper;
    Q_UNUSED(file); // már inicializált konstruktorban
    return helper;
}

QString FileNameHelper::getWorkingFolder() const {
    return _isTest ? _projectPath : QCoreApplication::applicationDirPath();
}

QString FileNameHelper::getStorageCsvFile() const {
    auto fn = QDir(_projectPath).filePath("storages.csv");
    return fn;
}

QString FileNameHelper::getMaterialCsvFile() const {
    auto fn = QDir(_projectPath).filePath("materials.csv");
    return fn;
}

QString FileNameHelper::getGroupCsvFile() const {
    auto fn = QDir(_projectPath).filePath("materialgroups.csv"); // vagy ahová ténylegesen rakod
    return fn;
}

QString FileNameHelper::getGroupMembersCsvFile() const {
    auto fn = QDir(_projectPath).filePath("materialgroup_members.csv"); // vagy ahová ténylegesen rakod
    return fn;
}

QString FileNameHelper::getStockCsvFile() const {
    auto fn = QDir(_projectPath).filePath("stock.csv"); // vagy ahová ténylegesen rakod
    return fn;
}

QString FileNameHelper::getLeftoversCsvFile() const {
    auto fn = QDir(_projectPath).filePath("leftovers.csv"); // vagy ahová ténylegesen rakod
    return fn;
}

QString FileNameHelper::getSettingsFilePath() const {
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);
    return dir.filePath("settings.ini");
}


QString FileNameHelper::generateTimestamp() const {
    return QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
}


QString FileNameHelper::combinePath(const QString& folder, const QString& fileName) const {
    return QDir(folder).filePath(fileName);
}


/*log*/

QString FileNameHelper::getNew_LogFileName() const {
    QString fn0 = QStringLiteral("log_%1.txt").arg(generateTimestamp());
    return fn0;
}

QString FileNameHelper::getLogFolder() const {
    auto fn = QDir(_projectPath).filePath("logs");
    return fn;
}

QString FileNameHelper::getLogFilePath(const QString& fn) const {
    return combinePath(getLogFolder(), fn);
}

/*cuttingplan*/
QString FileNameHelper::getNew_CuttingPlanFileName() const {
    QString fn0 = QStringLiteral("cuttingplan_%1.txt").arg(generateTimestamp());
    return fn0;
}

QString FileNameHelper::getCuttingPlanFolder() const {
    auto fn = QDir(_projectPath).filePath("cutting_plans");
    return fn;
}

QString FileNameHelper::getCuttingPlanFilePath(const QString fn) const {
    return combinePath(getCuttingPlanFolder(), fn);
}

/*movement*/

QString FileNameHelper::getMovementLogFileNameForDate(const QDate& date) const {
    return date.toString("yyyy-MM-dd") + ".log";
}

QString FileNameHelper::getMovementLogFilePathForDate(const QDate& date) const {
    return combinePath(getLogFolder(), getMovementLogFileNameForDate(date));
}

/*CuttingMachines*/

QString FileNameHelper::getCuttingMachineCsvFile() const {
    return QDir(_projectPath).filePath("cuttingmachines.csv");
}

QString FileNameHelper::getCuttingMachineMaterialsCsvFile() const {
    return QDir(_projectPath).filePath("cuttingmachine_materialtypes.csv");
}

/*RAL colors*/

/* RAL Colors */

QString FileNameHelper::getRalColorsFolder() const {
    auto fn = QDir(_projectPath).filePath("ral_colors");
    return fn;
}

QString FileNameHelper::getRalClassicCsvFile() const {
    return combinePath(getRalColorsFolder(), "classic.csv");
}

QString FileNameHelper::getRalDesignCsvFile() const {
    return combinePath(getRalColorsFolder(), "design.csv");
}

QString FileNameHelper::getRalPlastic1CsvFile() const {
    return combinePath(getRalColorsFolder(), "p1.csv");
}

QString FileNameHelper::getRalPlastic2CsvFile() const {
    return combinePath(getRalColorsFolder(), "p2.csv");
}

