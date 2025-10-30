#pragma once
#include <QString>

class TestManager {
public:
    static TestManager& instance();

    // Általános belépési pont a tesztekhez
    void runBusinessLogicTests(const QString& profile);

private:
    TestManager() = default;

    // Profil-specifikus tesztfüggvények
    void runDoubleTests();
    void runMakiTests();
    void runFullTests();
};
