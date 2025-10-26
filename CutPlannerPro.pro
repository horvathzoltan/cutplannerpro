QT       += core gui
#network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

CONFIG += c++20

DEFINES += TARGI=$$TARGET
message( "TARGET = "$$TARGI )
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    model/storageaudit/auditcontext.cpp \
    service/buildnumber.cpp \
    service/storageaudit/auditcontextbuilder.cpp \
    service/storageaudit/auditstatemanager.cpp \
    common/color/namedcolor.cpp \
    common/csvimporter.cpp \
    common/eventlogger.cpp \
    common/filehelper.cpp \
    common/filenamehelper.cpp \
    common/logger.cpp \
    common/quantityparser.cpp \
    common/settingsmanager.cpp \
    common/startup/startupmanager.cpp \
    view/tableutils/auditgrouplabeler.cpp \
    view/tableutils/auditgroupsynchronizer.cpp \
    view/tableutils/resulttable_rowstyler.cpp \
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
    model/relocation/relocationsourceentry.h \
    model/repositories/cuttingmachinerepository.cpp \
    model/repositories/cuttingrequestrepository.cpp \
    model/repositories/leftoverstockrepository.cpp \
    model/repositories/materialgrouprepository.cpp \
    model/repositories/materialrepository.cpp \
    model/repositories/stockrepository.cpp \
    model/repositories/storagerepository.cpp \
    model/stockentry.cpp \
    model/storageentry.cpp \
    model/storagetype.cpp \
    presenter/CuttingPresenter.cpp \
    service/cutting/optimizer/exporter.cpp \
    service/cutting/plan/autosavemanager.cpp \
    service/cutting/result/archivedwasteutils.cpp \
    service/cutting/result/leftoversourceutils.cpp \
    service/cutting/segment/segmentutils.cpp \
    service/relocation/relocationplanner.cpp \
    service/relocation/relocationservice.cpp \
    service/storageaudit/leftoverauditservice.cpp \
    service/storageaudit/storageauditservice.cpp \
    view/MainWindow.cpp \
    view/cutanalyticspanel.cpp \
    view/dialog/input/addinputdialog.cpp \
    view/dialog/waste/addwastedialog.cpp \
    view/dialog/movement/movementdialog.cpp \
    view/dialog/relocation/relocationquantitydialog.cpp \
    view/dialog/stock/addstockdialog.cpp \
    view/dialog/stock/editcommentdialog.cpp \
    view/dialog/stock/editquantitydialog.cpp \
    view/dialog/stock/editstoragedialog.cpp \
    view/managers/cuttinginstructiontable_manager.cpp \
    view/managers/inputtable_manager.cpp \
    view/managers/leftovertable_manager.cpp \
    view/managers/relocationplantable_manager.cpp \
    view/managers/resultstable_manager.cpp \
    view/managers/stocktable_manager.cpp \
    view/managers/storageaudittable_manager.cpp

HEADERS += \
    common/color/colorconstants.h \
    common/identifierutils.h \
    common/signalhelper.h \
    common/sysinfohelper.h \
    model/inventorysnapshot.h \
    model/machine/machineutils.h \
    model/material/material_utils.h \
    model/material/materialgroup_utils.h \
    model/relocation/relocationauditstatus.h \
    model/storageaudit/audit_enums.h \
    service/buildnumber.h \
    service/cutting/optimizer/optimizationauditbuilder.h \
    service/cutting/optimizer/optimizationlogger.h \
    service/cutting/optimizer/optimizationrunner.h \
    service/cutting/optimizer/optimizationviewupdater.h \
    service/cutting/optimizer/optimizerconstants.h \
    service/cutting/optimizer/optimizerutils.h \
    service/snapshot/inventorysnapshotbuilder.h \
    service/snapshot/requestsnapshotbuilder.h \
    service/storageaudit/auditcontextbuilder.h \
    service/storageaudit/auditstatemanager.h \
    service/storageaudit/auditsyncguard.h \
    common/color/namedcolor.h \
    common/csvhelper.h \
    common/csvimporter.h \
    common/eventlogger.h \
    common/filehelper.h \
    common/filenamehelper.h \
    common/logger.h \
    common/logmeta.h \
    common/movementdatahelper.h \
    common/qteventutil.h \
    common/quantityparser.h \
    common/scopedperthreadlock.h \
    common/styleprofiles/auditcolors.h \
    common/styleprofiles/cuttingcolors.h \
    common/styleprofiles/cuttingstatusutils.h \
    common/styleprofiles/relocationcolors.h \
    view/tablerowstyler/materialrowstyler.h \
    view/tableutils/RowTracker.h \
    view/tableutils/auditcellformatter.h \
    view/tableutils/auditgrouplabeler.h \
    view/tableutils/auditgroupsynchronizer.h \
    view/tableutils/colorutils.h \
    view/tableutils/rowid.h \
    view/tableutils/storageaudittable_connector.h \
    view/tableutils/storageaudittable_rowstyler.h \
    view/tableutils/tableutils_auditcells.h \
    model/cutting/cuttingmachine.h \
    model/cutting/instruction/cutinstruction.h \
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
    model/relocation/relocationinstruction.h \
    model/relocation/relocationquantitymodel.h \
    model/relocation/relocationquantityrow.h \
    model/relocation/relocationtargetentry.h \
    model/repositories/cuttingmachinerepository.h \
    model/storageaudit/auditcontext.h \
    model/storageaudit/auditcontext_text.h \
    model/storageaudit/auditgroupinfo.h \
    model/storageaudit/auditstatus.h \
    model/storageaudit/auditstatus_text.h \
    model/storageaudit/storageauditentry.h \
    model/storageaudit/storageauditrow.h \
    service/cutting/optimizer/exporter.h \
    service/cutting/plan/autosavemanager.h \
    service/cutting/result/archivedwasteutils.h \
    service/cutting/result/leftoversourceutils.h \
    service/cutting/result/resultutils.h \
    service/cutting/segment/segmentutils.h \
    service/movementlogger.h \
    common/settingsmanager.h \
    common/startup/startupmanager.h \
    common/stringify.h \
    view/tableutils/colorlogicutils.h \
    view/tableutils/inputtable_connector.h \
    view/tableutils/inputtable_rowstyler.h \
    view/tableutils/leftovertable_connector.h \
    view/tableutils/leftovertable_rowstyler.h \
    view/tableutils/resulttable_rowstyler.h \
    view/tableutils/stocktable_connector.h \
    view/tableutils/stocktable_rowstyler.h \
    view/tablerowstyler/tablestyleutils.h \
    view/tableutils/tableutils.h \
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
    service/relocation/relocationplanner.h \
    service/relocation/relocationservice.h \
    service/stockmovementservice.h \
    service/storageaudit/auditutils.h \
    service/storageaudit/leftoverauditservice.h \
    service/storageaudit/storageauditservice.h \
    view/MainWindow.h \
    view/columnindexes/tablecuttinginstruction_columns.h \
    view/eventloghelpers.h \
    view/managers/cuttinginstructiontable_manager.h \
    view/viewmodels/audit/cellgenerator.h \
    view/viewmodels/audit/rowgenerator.h \
    view/viewmodels/cutting/rowgenerator.h \
    view/viewmodels/relocation/cellgenerator.h \
    view/viewmodels/relocation/rowgenerator.h \
    view/cellhelpers/auditcelltooltips.h \
    view/columnidexes/relocationplantable_columns.h \
    view/cutanalyticspanel.h \
    view/dialog/input/addinputdialog.h \
    view/dialog/waste/addwastedialog.h \
    view/dialog/movement/movementdialog.h \
    view/dialog/relocation/relocationquantitydialog.h \
    view/dialog/stock/addstockdialog.h \
    view/dialog/stock/editcommentdialog.h \
    view/dialog/stock/editquantitydialog.h \
    view/dialog/stock/editstoragedialog.h \
    view/managers/inputtable_manager.h \
    view/managers/leftovertable_manager.h \
    view/managers/relocationplantable_manager.h \
    view/managers/resultstable_manager.h \
    view/managers/stocktable_manager.h \
    view/managers/storageaudittable_manager.h \
    view/tablehelpers/relocationquantityhelpers.h \
    view/tablehelpers/tablerowpopulator.h \
    view/viewmodels/tablecellviewmodel.h \
    view/viewmodels/tablerowviewmodel.h

FORMS += \
    view/MainWindow.ui \
    view/dialog/input/addinputdialog.ui \
    view/dialog/waste/addwastedialog.ui \
    view/dialog/movement/movementdialog.ui \
    view/dialog/relocation/relocationquantitydialog.ui \
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
    run.txt \
    testdata/stock.csv \
    testdata/storages.csv
