QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common/filenamehelper.cpp \
    common/rowstyler.cpp \
    common/startup/startupmanager.cpp \
    main.cpp \
    model/CuttingOptimizerModel.cpp \
    model/crosssectionshape.cpp \
    model/cutresult.cpp \
    model/cuttingmachine.cpp \
    model/cuttingrequest.cpp \
    model/identifiableentity.cpp \
    model/materialgroup.cpp \
    model/materialmaster.cpp \
    model/materialtype.cpp \
    model/registries/materialgroupregistry.cpp \
    model/registries/materialregistry.cpp \
    model/registries/reusablestockregistry.cpp \
    model/registries/stockregistry.cpp \
    model/repositories/materialgrouprepository.cpp \
    model/repositories/materialrepository.cpp \
    model/repositories/reusablestockrepository.cpp \
    model/repositories/stockrepository.cpp \
    model/reusablestockentry.cpp \
    model/stockentry.cpp \
    presenter/CuttingPresenter.cpp \
    view/MainWindow.cpp \
    view/dialog/addinputdialog.cpp \
    view/managers/inputtablemanager.cpp \
    view/managers/leftovertablemanager.cpp \
    view/managers/stocktablemanager.cpp

HEADERS += \
    common/colorutils.h \
    common/cutresultutils.h \
    common/filenamehelper.h \
    common/grouputils.h \
    common/materialutils.h \
    common/rowstyler.h \
    common/startup/startupmanager.h \
    common/stringify.h \
    model/CuttingOptimizerModel.h \
    model/crosssectionshape.h \
    model/cutresult.h \
    model/cuttingmachine.h \
    model/cuttingrequest.h \
    model/identifiableentity.h \
    model/materialgroup.h \
    model/materialmaster.h \
    model/materialtype.h \
    model/registries/materialgroupregistry.h \
    model/registries/materialregistry.h \
    model/registries/reusablestockregistry.h \
    model/registries/stockregistry.h \
    model/repositories/materialgrouprepository.h \
    model/repositories/materialrepository.h \
    model/repositories/reusablestockrepository.h \
    model/repositories/stockrepository.h \
    model/reusablestockentry.h \
    model/stockentry.h \
    presenter/CuttingPresenter.h \
    view/MainWindow.h \
    view/dialog/addinputdialog.h \
    view/managers/inputtablemanager.h \
    view/managers/leftovertablemanager.h \
    view/managers/stocktablemanager.h

FORMS += \
    view/MainWindow.ui \
    view/dialog/addinputdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    README.md \
    testdata/groups.csv \
    testdata/leftovers.csv \
    testdata/materials.csv \
    testdata/stock.csv
