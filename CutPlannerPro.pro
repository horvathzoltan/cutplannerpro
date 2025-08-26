QT       += core gui
#network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common/color/namedcolor.cpp \
    common/csvimporter.cpp \
    common/filehelper.cpp \
    common/filenamehelper.cpp \
    common/logger.cpp \
    common/quantityparser.cpp \
    common/settingsmanager.cpp \
    common/startup/startupmanager.cpp \
    common/tableutils/resulttable_rowstyler.cpp \
    main.cpp \
    model/archivedwasteentry.cpp \
    model/crosssectionshape.cpp \
    model/cutting/cuttingmachine.cpp \
    model/cutting/optimizer/optimizermodel.cpp \
    model/cutting/piece/pieceinfo.cpp \
    model/cutting/piece/piecewithmaterial.cpp \
    model/cutting/plan/cutplan.cpp \
    model/cutting/plan/request.cpp \
    model/cutting/result/resultmodel.cpp \
    model/identifiableentity.cpp \
    model/leftoverstockentry.cpp \
    model/material/materialgroup.cpp \
    model/material/materialmaster.cpp \
    model/material/materialtype.cpp \
    model/picking/pickingcomparisonresult.cpp \
    model/picking/pickingitem.cpp \
    model/registries/cuttingmachineregistry.cpp \
    model/registries/cuttingplanrequestregistry.cpp \
    model/registries/leftoverstockregistry.cpp \
    model/registries/materialgroupregistry.cpp \
    model/registries/materialregistry.cpp \
    model/registries/stockregistry.cpp \
    model/registries/storageregistry.cpp \
    model/repositories/cuttingmachinerepository.cpp \
    model/repositories/cuttingrequestrepository.cpp \
    model/repositories/leftoverstockrepository.cpp \
    model/repositories/materialgrouprepository.cpp \
    model/repositories/materialrepository.cpp \
    model/repositories/stockrepository.cpp \
    model/repositories/storagerepository.cpp \
    model/stockentry.cpp \
    model/storageaudit/storageauditentry.cpp \
    model/storageentry.cpp \
    model/storagetype.cpp \
    presenter/CuttingPresenter.cpp \
    service/cutting/optimizer/exporter.cpp \
    service/cutting/plan/autosavemanager.cpp \
    service/cutting/plan/finalizer.cpp \
    service/cutting/result/archivedwasteutils.cpp \
    service/cutting/result/leftoversourceutils.cpp \
    service/cutting/segment/segmentutils.cpp \
    service/storageaudit/storageauditservice.cpp \
    view/MainWindow.cpp \
    view/cutanalyticspanel.cpp \
    view/dialog/addinputdialog.cpp \
    view/dialog/addwastedialog.cpp \
    view/dialog/movement/movementdialog.cpp \
    view/dialog/stock/addstockdialog.cpp \
    view/dialog/stock/editcommentdialog.cpp \
    view/dialog/stock/editquantitydialog.cpp \
    view/dialog/stock/editstoragedialog.cpp \
    view/managers/inputtablemanager.cpp \
    view/managers/leftovertablemanager.cpp \
    view/managers/resultstablemanager.cpp \
    view/managers/stocktablemanager.cpp \
    view/managers/storageaudit_tablemanager.cpp

HEADERS += \
    common/color/namedcolor.h \
    common/csvhelper.h \
    common/csvimporter.h \
    common/filehelper.h \
    common/filenamehelper.h \
    common/grouputils.h \
    common/logger.h \
    common/logmeta.h \
    common/materialutils.h \
    common/qteventutil.h \
    common/quantityparser.h \
    common/tableutils/colorutils.h \
    model/cutting/cuttingmachine.h \
    model/cutting/optimizer/optimizermodel.h \
    model/cutting/piece/pieceinfo.h \
    model/cutting/piece/piecewithmaterial.h \
    model/cutting/plan/cutplan.h \
    model/cutting/plan/request.h \
    model/cutting/plan/source.h \
    model/cutting/plan/status.h \
    model/cutting/result/leftoversource.h \
    model/cutting/result/resultmodel.h \
    model/cutting/result/resultsource.h \
    model/cutting/segment/segmentmodel.h \
    model/material/materialgroup.h \
    model/material/materialmaster.h \
    model/material/materialtype.h \
    model/picking/pickingcomparisonresult.h \
    model/picking/pickingitem.h \
    model/registries/cuttingmachineregistry.h \
    model/repositories/cuttingmachinerepository.h \
    model/storageaudit/storageauditentry.h \
    service/cutting/optimizer/exporter.h \
    service/cutting/plan/autosavemanager.h \
    service/cutting/plan/finalizer.h \
    service/cutting/result/archivedwasteutils.h \
    service/cutting/result/leftoversourceutils.h \
    service/cutting/result/resultutils.h \
    service/cutting/segment/segmentutils.h \
    service/movementlogger.h \
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
    model/archivedwasteentry.h \
    model/crosssectionshape.h \
    model/identifiableentity.h \
    model/leftoverstockentry.h \
    model/movementdata.h \
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
    model/stockentry.h \
    model/storageentry.h \
    model/storagetype.h \
    presenter/CuttingPresenter.h \
    service/movementlogmodel.h \
    service/stockmovementservice.h \
    service/storageaudit/storageauditservice.h \
    view/MainWindow.h \
    view/cutanalyticspanel.h \
    view/dialog/addinputdialog.h \
    view/dialog/addwastedialog.h \
    view/dialog/movement/movementdialog.h \
    view/dialog/stock/addstockdialog.h \
    view/dialog/stock/editcommentdialog.h \
    view/dialog/stock/editquantitydialog.h \
    view/dialog/stock/editstoragedialog.h \
    view/managers/inputtablemanager.h \
    view/managers/leftovertablemanager.h \
    view/managers/resultstablemanager.h \
    view/managers/stocktablemanager.h \
    view/managers/storageaudit_tablemanager.h

FORMS += \
    view/MainWindow.ui \
    view/dialog/addinputdialog.ui \
    view/dialog/addwastedialog.ui \
    view/dialog/movement/movementdialog.ui \
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
    testdata/csvlist.sh \
    testdata/cutting_plans/cutting_plan_1.csv \
    testdata/cuttingmachine_materialtypes.csv \
    testdata/cuttingmachines.csv \
    testdata/leftovers.csv \
    testdata/materialgroup_members.csv \
    testdata/materialgroups.csv \
    testdata/materials.csv \
    testdata/ral_colors/classic.csv \
    testdata/ral_colors/design.csv \
    testdata/ral_colors/p1.csv \
    testdata/ral_colors/p2.csv \
    testdata/stock.csv \
    testdata/storages.csv
