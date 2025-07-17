#include "view/MainWindow.h"

#include <QApplication>
#include "common/filenamehelper.h"
#include <model/materialregistry.h>
#include <model/materialrepository.h>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // itt initelünk mindet
    //FileNameHelper::Init();

    auto& helper = FileNameHelper::instance();

    if(helper.isInited()){
        QString csvPath = helper.getMaterialCsvFile();

        auto& registry = MaterialRegistry::instance();
        MaterialRepository::loadFromCSV(registry);

        QString logName = helper.getLogFileName();
    }

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
