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
            _projectPath = dir.absolutePath();
            return true;
        }
    }
    // ❌ Hiba logolható itt, ha szükséges

    _projectPath = QCoreApplication::applicationDirPath();

    QDir fallbackTestDir(_projectPath);
    if (!fallbackTestDir.exists("testdata")) {
        qWarning("Fallback aktiválva, de nincs 'testdata' mappa a bináris mappában: %s", qUtf8Printable(_projectPath));

            qWarning("Aktuális bináris mappa: %s", qUtf8Printable(_projectPath));
        return false;
    }

    // ✅ fallback sikeres és valid
    qDebug("✅ Fallback init sikeres, használjuk: %s", qUtf8Printable(_projectPath));
    return true;
#endif
}

FileNameHelper& FileNameHelper::instance(const char* file) {
    static FileNameHelper helper;
    Q_UNUSED(file); // már inicializált konstruktorban
    return helper;
}

QString FileNameHelper::getTestFolderPath() const {
    return QDir(_projectPath).filePath("testdata");
}

QString FileNameHelper::getWorkingFolder() const {
    return _isTest ? getTestFolderPath() : QCoreApplication::applicationDirPath();
}

QString FileNameHelper::getMaterialCsvFile() const {
    return QDir(getTestFolderPath()).filePath("materials.csv");
}

QString FileNameHelper::getLogFileName() const {
    return QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
}
