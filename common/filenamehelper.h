#pragma once

#include <QString>

//3. 🧠 Lehetőség singletonná alakításra
// 📁 Tesztfájl elérési segédfüggvények
class FileNameHelper{
private:
    QString _projectPath;
    bool _isTest = false;
    bool _isInited = false;

    // 🛡️ Privát konstruktor
    FileNameHelper();
    bool init(const char* file);
public:
    // 🔁 Példány elérése
    static FileNameHelper& instance(const char* file = __FILE__);

    bool isTestMode() const { return _isTest; }
    bool isInited() const { return _isInited; }

    // ⚙️ Beállítások
    void setTestMode(bool v) { _isTest = v; }

    // 📁 Elérési utak
    //QString getTestFolderPath() const;
    QString getWorkingFolder() const;
    QString getMaterialCsvFile() const; // 📁 Anyag törzs CSV útvonal
    QString getGroupCsvFile() const; // 📁 Csoportdefiníciós CSV útvonal
    QString getStockCsvFile() const; // 📁 Készlet CSV útvonal

    // 📓 Naplófájl név
    QString getLogFileName() const;
    QString getLeftoversCsvFile() const;
};
