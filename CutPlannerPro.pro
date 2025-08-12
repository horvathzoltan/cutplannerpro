QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common/archivedwasteutils.cpp \
    common/csvimporter.cpp \
    common/cuttingplanfinalizer.cpp \
    common/filehelper.cpp \
    common/filenamehelper.cpp \
    common/leftoversourceutils.cpp \
    common/logger.cpp \
    common/optimizationexporter.cpp \
    common/planautosavemanager.cpp \
    common/quantityparser.cpp \
    common/segmentutils.cpp \
    common/settingsmanager.cpp \
    common/startup/startupmanager.cpp \
    common/tableutils/resulttable_rowstyler.cpp \
    main.cpp \
    model/CuttingOptimizerModel.cpp \
    model/archivedwasteentry.cpp \
    model/crosssectionshape.cpp \
    model/cutplan.cpp \
    model/cutresult.cpp \
    model/cutting/piecewithmaterial.cpp \
    model/cuttingmachine.cpp \
    model/cuttingplanrequest.cpp \
    model/identifiableentity.cpp \
    model/leftoverstockentry.cpp \
    model/materialgroup.cpp \
    model/materialmaster.cpp \
    model/materialtype.cpp \
    model/pieceinfo.cpp \
    model/registries/cuttingplanrequestregistry.cpp \
    model/registries/leftoverstockregistry.cpp \
    model/registries/materialgroupregistry.cpp \
    model/registries/materialregistry.cpp \
    model/registries/stockregistry.cpp \
    model/registries/storageregistry.cpp \
    model/repositories/cuttingrequestrepository.cpp \
    model/repositories/leftoverstockrepository.cpp \
    model/repositories/materialgrouprepository.cpp \
    model/repositories/materialrepository.cpp \
    model/repositories/stockrepository.cpp \
    model/repositories/storagerepository.cpp \
    model/segment.cpp \
    model/stockentry.cpp \
    model/storageentry.cpp \
    model/storagetype.cpp \
    presenter/CuttingPresenter.cpp \
    view/MainWindow.cpp \
    view/cutanalyticspanel.cpp \
    view/dialog/addinputdialog.cpp \
    view/dialog/addwastedialog.cpp \
    view/dialog/stock/addstockdialog.cpp \
    view/dialog/stock/editcommentdialog.cpp \
    view/dialog/stock/editquantitydialog.cpp \
    view/dialog/stock/editstoragedialog.cpp \
    view/managers/inputtablemanager.cpp \
    view/managers/leftovertablemanager.cpp \
    view/managers/resultstablemanager.cpp \
    view/managers/stocktablemanager.cpp

HEADERS += \
    common/archivedwasteutils.h \
    common/colorutils.h \
    common/csvimporter.h \
    common/cutresultutils.h \
    common/cuttingplanfinalizer.h \
    common/filehelper.h \
    common/filenamehelper.h \
    common/grouputils.h \
    common/leftoversourceutils.h \
    common/logger.h \
    common/materialutils.h \
    common/optimizationexporter.h \
    common/planautosavemanager.h \
    common/qteventutil.h \
    common/quantityparser.h \
    common/segmentutils.h \
    common/settingsmanager.h \
    common/startup/startupmanager.h \
    common/stringify.h \
    common/tableutils/colorconstants.h \
    common/tableutils/colorlogicutils.h \
    common/tableutils/inputtable_connector.h \
    common/tableutils/inputtable_rowstyler.h \
    common/tableutils/leftovertable_connector.h \
    common/tableutils/leftovertable_rowstyler.h \
    common/tableutils/resulttable_rowstyler.h \
    common/tableutils/stocktable_connector.h \
    common/tableutils/stocktable_rowstyler.h \
    common/tableutils/tablestyleutils.h \
    common/tableutils/tableutils.h \
    model/CuttingOptimizerModel.h \
    model/archivedwasteentry.h \
    model/crosssectionshape.h \
    model/cutplan.h \
    model/cutresult.h \
    model/cutting/piecewithmaterial.h \
    model/cuttingmachine.h \
    model/cuttingplanrequest.h \
    model/identifiableentity.h \
    model/leftoverstockentry.h \
    model/materialgroup.h \
    model/materialmaster.h \
    model/materialtype.h \
    model/pieceinfo.h \
    model/registries/cuttingplanrequestregistry.h \
    model/registries/leftoverstockregistry.h \
    model/registries/materialgroupregistry.h \
    model/registries/materialregistry.h \
    model/registries/stockregistry.h \
    model/registries/storageregistry.h \
    model/repositories/cuttingrequestrepository.h \
    model/repositories/leftoverstockrepository.h \
    model/repositories/materialgrouprepository.h \
    model/repositories/materialrepository.h \
    model/repositories/stockrepository.h \
    model/repositories/storagerepository.h \
    model/segment.h \
    model/stockentry.h \
    model/storageentry.h \
    model/storagetype.h \
    presenter/CuttingPresenter.h \
    view/MainWindow.h \
    view/cutanalyticspanel.h \
    view/dialog/addinputdialog.h \
    view/dialog/addwastedialog.h \
    view/dialog/stock/addstockdialog.h \
    view/dialog/stock/editcommentdialog.h \
    view/dialog/stock/editquantitydialog.h \
    view/dialog/stock/editstoragedialog.h \
    view/managers/inputtablemanager.h \
    view/managers/leftovertablemanager.h \
    view/managers/resultstablemanager.h \
    view/managers/stocktablemanager.h

FORMS += \
    view/MainWindow.ui \
    view/dialog/addinputdialog.ui \
    view/dialog/addwastedialog.ui \
    view/dialog/stock/addstockdialog.ui \
    view/dialog/stock/editcommentdialog.ui \
    view/dialog/stock/editquantitydialog.ui \
    view/dialog/stock/editstoragedialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    README.md \
    testdata/cutting_plans/cutting_plan_1.csv \
    testdata/groups.csv \
    testdata/leftovers.csv \
    testdata/materials.csv \
    testdata/stock.csv \
    testdata/storages.csv
