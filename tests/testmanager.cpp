#include "testmanager.h"
#include "common/logger.h"
#include "common/identifierutils.h"

#include <QList>

TestManager& TestManager::instance() {
    static TestManager _instance;
    return _instance;
}

void TestManager::runBusinessLogicTests(const QString& profile) {
    zInfo(QString("🧪 Tesztprofil kiválasztva: %1").arg(profile));
    if (profile == "maki") {
        runMakiTests();
    } else if (profile == "full") {
        runFullTests();
    } else {
        zWarning(QString("⚠️ Ismeretlen tesztprofil: %1").arg(profile));
    }
}

void TestManager::runMakiTests() {
    zInfo("▶️ Maki tesztprofil futtatása...");

    // 🔢 Tesztértékek: 2, 3, 4 és 5 jegyű számok
    QList<int> matIds = {42, 423, 999, 1000, 1234, 22999, 23000, 24000, 25000};
    QList<int> rstIds = {42, 423, 999, 1000, 1234, 22999, 23000, 24000, 25000};

    zInfo("=== Material ID tesztek ===");
    for (int id : matIds) {
        QString code = IdentifierUtils::makeMaterialId(id);
        zInfo(QString("MAT input=%1 → %2").arg(id).arg(code));
    }

    zInfo("=== Leftover ID tesztek ===");
    for (int id : rstIds) {
        QString code = IdentifierUtils::makeLeftoverId(id);
        zInfo(QString("RST input=%1 → %2").arg(id).arg(code));
    }
}


void TestManager::runFullTests() {
    zInfo("▶️ Full tesztprofil futtatása...");
    // Ide jöhetnek komplexebb integrációs tesztek
}
