#include "view/MainWindow.h"

#include <QApplication>
#include <QMessageBox>

#include <model/registries/materialregistry.h>
#include <model/repositories/materialrepository.h>
#include <common/startup/startupmanager.h>
#include <common/settingsmanager.h>
#include "common/logger.h"

#include <common/eventlogger.h>

int main(int argc, char *argv[])
{
    // itt initelünk mindet
    Logger::Init(Logger::ErrLevel::INFO, Logger::DbgLevel::TRACE, false, false);
    SettingsManager::instance().load();

    // 🔧 Eseménynapló fájl megnyitása még az init előtt
    EventLogger::instance().setLogFile("eventlog.txt");

    // elvileg ha gond van, akkor itt nem nyitjuk megh a főablakot
    QApplication app(argc, argv);

    StartupManager manager;
    StartupStatus status = manager.runStartupSequence();

    if (!status.isSuccess())  {
        QMessageBox::critical(nullptr, "Indítási hiba", status.errorMessage());
        return -1;
    }

    if (!status.warnings().isEmpty()) {
        QMessageBox::warning(nullptr, "Figyelmeztetés",
                             "Az alkalmazás elindult, de a következő problémák felmerültek:\n\n" +
                                 status.warnings().join("\n"));
    }

    MainWindow window;
//    window.setWindowTitle("CutPlanner MVP");
//    window.resize(1000, 600); // Opcionális kezdeti méret
    window.show();

    return app.exec();
}

