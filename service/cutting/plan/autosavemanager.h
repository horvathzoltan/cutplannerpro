// #pragma once

// #include <QString>
// #include <QSettings>

// class PlanAutosaveManager {
// public:
//     PlanAutosaveManager();

//     bool loadLastPlan();               // 🔁 Visszatöltés induláskor
//     void saveCurrentPlan();            // 💾 Mentés aktuális állapotban
//     void updatePlan(const QString& path); // 📝 Fájlútvonal beállítása

//     QString getCurrentPath() const;
//     bool hasActivePath() const;

// private:
//     QString currentPath;
//     QSettings settings;

//     QString generateAutosavePath() const;
// };
