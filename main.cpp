#include "view/MainWindow.h"

#include <QApplication>
#include <QMessageBox>
#include "common/filenamehelper.h"
#include <model/registries/materialregistry.h>
#include <model/repositories/materialrepository.h>
#include <common/startup/startupmanager.h>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // itt initelünk mindet
    //FileNameHelper::Init();

    //auto& helper = FileNameHelper::instance();

    StartupManager manager;
    StartupStatus status = manager.runStartupSequence();

    if (!status.ok) {
        QMessageBox::critical(nullptr, "Indítási hiba", status.errorMessage);
        return -1;
    }

    if (!status.warnings.isEmpty()) {
        QMessageBox::warning(nullptr, "Figyelmeztetés",
                             "Az alkalmazás elindult, de a következő problémák felmerültek:\n\n" +
                                 status.warnings.join("\n"));
    }


    // if(helper.isInited()){
    //     QString csvPath = helper.getMaterialCsvFile();

    //     auto& registry = MaterialRegistry::instance();
    //     MaterialRepository::loadFromCSV(registry);

    //     QString logName = helper.getLogFileName();
    // }

    MainWindow window;
    window.setWindowTitle("CutPlanner MVP");
    window.resize(1000, 600); // Opcionális kezdeti méret
    window.show();

    return app.exec();
}

/*
src/
├── model/
│   └── CuttingOptimizerModel.h/.cpp
├── presenter/
│   └── CuttingPresenter.h/.cpp
├── view/
│   └── MainWindow.ui
│   └── MainWindow.h/.cpp
└── main.cpp
*/
