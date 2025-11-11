#include "testmanager.h"
#include "common/logger.h"
#include "common/identifierutils.h"

#include <QList>

#include <model/cutting/plan/request.h>

TestManager& TestManager::instance() {
    static TestManager _instance;
    return _instance;
}

void TestManager::runBusinessLogicTests(const QString& profile) {
    zInfo(QString("🧪 Tesztprofil kiválasztva: %1").arg(profile));
    if (profile == "double") {
        runDoubleTests();
    } else
    if (profile == "maki") {
        runMakiTests();
    } else if (profile == "full") {
        runFullTests();    
    } else if (profile == "tolerance") {
        runToleranceTests();
    } else {
        zWarning(QString("⚠️ Ismeretlen tesztprofil: %1").arg(profile));
    }
}

void TestManager::runDoubleTests(){
    double a = 1250.0;
    double b = 288.49;
    double c = 288.5;

    QString a_txt= QString("%1").arg(a, 0, 'f', 0); // "1250"
    QString b_txt= QString("%1").arg(b, 0, 'f', 0); // "288"
    QString c_txt= QString("%1").arg(c, 0, 'f', 0); // "289"

    zInfo("a_txt:"+a_txt);
    zInfo("b_txt:"+b_txt);
    zInfo("c_txt:"+c_txt);
}

void TestManager::runToleranceTests()
{
    auto t1 = Tolerance::fromString("+/-0.5");    // min=-0.5, max=+0.5
    auto t2 = Tolerance::fromString("-0.5/0");    // min=-0.5, max=0
    auto t3 = Tolerance::fromString("234 ±0.5");  // min=-0.5, max=+0.5
    auto t4 = Tolerance::fromString("234 -0.5/+0.2"); // min=-0.5, max=+0.2

    zInfo("t1:"+t1->toString());           // "+/-0.5 mm"
    zInfo("t2:"+t2->toString());           // "-0.5/0 mm"
    zInfo("t3:"+t3->toString(234));        // "234 ±0.5 mm"
    zInfo("t4:"+t4->toString(234));        // "234 -0.5/0.2 mm"
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

    zInfo("=== Rod ID tesztek ===");
    QList<int> rodIds = {
        42,       // Rod-042
        999,      // Rod-999
        1000,     // Rod-A-000
        1234,     // Rod-B-234
        22999,    // Rod-Z-999
        23000,    // Rod-AA-000
        24000,    // Rod-AB-000
        506999,   // Rod-ZZ-999
        507000,   // Rod-AAA-000
        10000000, // Rod-1A3F (hex fallback)
    };

    for (int id : rodIds) {
        QString code = IdentifierUtils::makeRodId(id);
        zInfo(QString("ROD input=%1 → %2").arg(id).arg(code));
    }
}


void TestManager::runFullTests() {
    zInfo("▶️ Full tesztprofil futtatása...");
    // Ide jöhetnek komplexebb integrációs tesztek
}
