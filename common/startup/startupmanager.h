#pragma once

#include <QString>
#include <QStringList>


// 🌱 Indulási állapotot leíró struktúra
struct StartupStatus {
    bool ok = false;
    QString errorMessage;
    QStringList warnings;

    static StartupStatus success() {
        return { true, QString(), {} };
    }

    static StartupStatus failure(const QString& msg) {
        return { false, msg, {} };
    }

    void addWarning(const QString& warning) {
        warnings.append(warning);
    }
};

// 🚀 Indítási logikát vezérlő osztály
class StartupManager {
public:
    StartupStatus runStartupSequence();

private:

    bool hasMinimumMaterials(int minCount);

    StartupStatus initMaterialRegistry();
    StartupStatus initMaterialGroupRegistry();
    StartupStatus initStockRegistry();
    StartupStatus initCuttingRequestRegistry();
    StartupStatus initReusableStockRegistry();
    StartupStatus initStorageRegistry();
    StartupStatus initCuttingMachineRegistry(); // ✂️ Vágógépek betöltése

    StartupStatus initRalColors();
};
