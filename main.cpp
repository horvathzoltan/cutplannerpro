#include "view/MainWindow.h"

#include <QApplication>
#include <QMessageBox>

#include <model/registries/materialregistry.h>
#include <model/repositories/materialrepository.h>
#include <common/startup/startupmanager.h>
#include <common/settingsmanager.h>
#include "common/logger.h"

#include <common/eventlogger.h>
#include <common/signalhelper.h>
#include <common/sysinfohelper.h>

#include <QDebug>
#include "tests/testmanager.h"

#define TEST_MODE

int main(int argc, char *argv[])
{
    // induláskor
    SignalHelper::setCleanupHandler([](int sig){
        EventLogger::instance().zEvent_(EventLogger::Info,
                                       QString("🛠️ Alkalmazás leállítása, jel: %1").arg(sig));

        if (sig == SignalHelper::SIGINT_) {
            qDebug().noquote() << "Ctrl+C megszakítás → gyors mentés";
        } else if (sig == SignalHelper::SIGTERM_) {
            qDebug().noquote() << "Killall → teljes cleanup";
        }
        // registry flush, fájlmentés, stb.
    });


    SignalHelper::setShutDownSignal(SignalHelper::SIGINT_); // shut down on ctrl-c
    SignalHelper::setShutDownSignal(SignalHelper::SIGTERM_); // shut down on killall

    QCoreApplication::setApplicationName(SysInfoHelper::instance().target());
    QCoreApplication::setApplicationVersion(Buildnumber::value);
    QCoreApplication::setOrganizationName("horvathzoltan");
    QCoreApplication::setOrganizationDomain("https://github.com/horvathzoltan");

    // itt initelünk mindet
    Logger::Init(Logger::ErrLevel::INFO, Logger::DbgLevel::TRACE, false, false);
    SettingsManager::instance().load(argc, argv);

    // --test maki
    if (SettingsManager::instance().isTestMode()) {
        TestManager::instance().runBusinessLogicTests(SettingsManager::instance().testProfile());
        return 0;
    }

    // 🔧 Eseménynapló fájl megnyitása még az init előtt
    EventLogger::instance().setLogFile("eventlog.txt");

    //SysInfoHelper::instance().setTarget(target, Buildnumber::_value);

    auto sysInfo = SysInfoHelper::instance().sysInfo();
    zInfo(sysInfo);
    zEvent(sysInfo);

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

