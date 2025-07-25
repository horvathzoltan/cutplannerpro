#include "filenamehelper.h"
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

QString FileNameHelper::getLogFileName() const {
    return QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
}

QString FileNameHelper::getMaterialCsvFile() const {
    auto fn = QDir(_projectPath).filePath("materials.csv");
    return fn;
}

QString FileNameHelper::getGroupCsvFile() const {
    auto fn = QDir(_projectPath).filePath("groups.csv"); // vagy ahová ténylegesen rakod
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

