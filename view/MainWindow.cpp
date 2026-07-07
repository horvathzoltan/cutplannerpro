#include "MainWindow.h"
#include "settings/settingsdialog.h"
#include "tableutils/highlightdelegate.h"
#include "tableutils/storageaudittable_connector.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QMessageBox>
//#include <QObject>
#include <QCloseEvent>
#include <QTimer>
#include <QFileDialog>
#include <QCheckBox>
#include <QShortcut>

//#include "cutanalyticspanel.h"
#include "../model/stockentry.h"
#include "../model/cutting/plan/request.h"

#include "../common/filenamehelper.h"
#include "settings/settingsmanager.h"
#include "tableutils/leftovertable_connector.h"
#include "tableutils/inputtable_connector.h"
#include "tableutils/stocktable_connector.h"
#include "../common/qteventutil.h"

#include "dialog/stock/addstockdialog.h"
#include "dialog/input/addinputdialog.h"

#include "../model/relocation/relocationinstruction.h"
#include "eventloghelpers.h"

#include "../service/relocation/relocationplanner.h"

#include "../common/eventlogger.h"
#include "view/MainWindowUIBuilder.h"

#include <view/dialog/materialfinder/materialfinderdialog.h>

#include "view/dialog/input/series_matrix_view.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initEventLogWidget();

    _actSeriesMatrix = new QAction(tr("📊 Sorozat mátrix"), this);
    _actSeriesMatrix->setShortcut(QKeySequence("Ctrl+M"));
    _actSeriesMatrix->setCheckable(true);
    _actSeriesMatrix->setChecked(false);

    ActionConnectorModel m1{
        .actMaterialFinder = MainWindowUIBuilder::createMaterialFinderAction(this),
        .actSettings = MainWindowUIBuilder::createSettingsAction(this),
        .actSeriesMatrix   =   _actSeriesMatrix // 🔹 egyszerű action
    };

    // ui->actionSeriesMatrix->setIcon(QIcon(":/icons/table_on.png"));
    // ui->actionSeriesMatrix->setText("📊 Mátrix");



    mainToolbarBuilder(m1);
    ActionConnector_connect(m1);

    ui->relocateQuickList->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    setWindowTitle("CutPlanner MVP");

    ui->tableInput->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableStock->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableLeftovers->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableStorageAudit->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableRelocationOrder->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    presenter = new CuttingPresenter(this, this);
    stockPresenter = new StockPresenter(this, this);

    connect(stockPresenter, &StockPresenter::highlightLeftover,
            this, &MainWindow::onHighlightLeftover);

    connect(stockPresenter, &StockPresenter::highlightStock,
            this, &MainWindow::onHighlightStock);

    connect(stockPresenter, &StockPresenter::showNotFoundMessage,
            this, &MainWindow::onShowNotFoundMessage);



    inputTableManager = std::make_unique<InputTableManager>(ui->tableInput, this);
    stockTableManager = std::make_unique<StockTableManager>(ui->tableStock, this);
    leftoverTableManager = std::make_unique<LeftoverTableManager>(ui->tableLeftovers, this);
    resultsTableManager = std::make_unique<ResultsTableManager>(ui->tableResults, this);
    storageAuditTableManager = std::make_unique<StorageAuditTableManager>(ui->tableStorageAudit, this);
    relocationPlanTableManager = std::make_unique<RelocationPlanTableManager>(ui->tableRelocationOrder, presenter, this);
    cuttingInstructionTableManager = std::make_unique<CuttingInstructionTableManager>(ui->tableCuttingInstruction, this);

    // 🔦 Sorvezető delegate bekötése
    _highlightDelegate = new HighlightDelegate(ui->tableCuttingInstruction);
    _highlightDelegate->currentRowIx =
        cuttingInstructionTableManager->currentRowIx();
    ui->tableCuttingInstruction->setItemDelegate(_highlightDelegate);

    connect(cuttingInstructionTableManager.get(),
            &CuttingInstructionTableManager::rowFinalized,
            this, &MainWindow::onRowFinalized);

    connect(cuttingInstructionTableManager.get(),
            &CuttingInstructionTableManager::compensationChanged,
            this, &MainWindow::onCompensationChanged);

    // connect(cuttingInstructionTableManager.get(), &CuttingInstructionTableManager::rowFinalized,
    //         this, [this, highlightDelegate](int rowIx) {
    //             highlightDelegate->completedRows.insert(rowIx);
    //             highlightDelegate->currentRowIx = cuttingInstructionTableManager->currentRowIx();
    //             ui->tableCuttingInstruction->viewport()->update();
    //         });

    InputTableConnector::Connect(this, inputTableManager.get(), presenter);
    StockTableConnector::Connect(this, stockTableManager.get(), presenter);
    LeftoverTableConnector::Connect(this, leftoverTableManager.get(), presenter);
    StorageAuditTableConnector::Connect(this, storageAuditTableManager.get(), presenter);
    ButtonConnector_Connect();//::Connect(ui, this);

    ui->tableResults->setAlternatingRowColors(true);
    ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableResults->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 📥 betöltött adatok megjelenítése
    inputTableManager->refresh_TableFromRegistry();         // Feltölti a tableInput-ot
    stockTableManager->refresh_TableFromRegistry();  // Frissíti a tableStock a StockRepository alapján
    leftoverTableManager->refresh_TableFromRegistry();  // Feltölti a maradékokat tesztadatokkal

    //inputFileName
    QString currentFileName = SettingsManager::instance().cuttingPlanFileName();
    QString fullPath = FileNameHelper::instance().getCuttingPlanFilePath(currentFileName);

    if (!currentFileName.isEmpty()) {
        setInputFileLabel(currentFileName, fullPath);
    }

    //analyticsPanel = new CutAnalyticsPanel(this);
    //ui->midLayout->addWidget(analyticsPanel);

    // 🔄 Fejléc állapot betöltése

    // oszlopszélesség
    ui->tableInput->horizontalHeader()->restoreState(SettingsManager::instance().inputTableHeaderState());
    ui->tableResults->horizontalHeader()->restoreState(SettingsManager::instance().resultsTableHeaderState());
    ui->tableStock->horizontalHeader()->restoreState(SettingsManager::instance().stockTableHeaderState());
    ui->tableLeftovers->horizontalHeader()->restoreState(SettingsManager::instance().leftoversTableHeaderState());
    ui->tableStorageAudit->horizontalHeader()->restoreState(SettingsManager::instance().storageAuditTableHeaderState());
    ui->tableRelocationOrder->horizontalHeader()->restoreState(SettingsManager::instance().relocationOrderTableHeaderState());
    ui->tableCuttingInstruction->horizontalHeader()->restoreState(SettingsManager::instance().cuttingInstructionTableHeaderState());
    // splitter
    ui->mainSplitter->restoreState(SettingsManager::instance().mainSplitterState());

    // ablakméret - az esemény időzítve (Qt event queue-ban)
    QtEventUtil::post(this, [this]() {
        restoreGeometry(SettingsManager::instance().windowGeometry());
        ui->mainSplitter->restoreState(SettingsManager::instance().mainSplitterState());
    });

    connect(presenter->auditStateManager(), &AuditStateManager::auditStateChanged,
            this, [this](AuditStateManager::AuditOutdatedReason reason) {
                switch (reason) {
                case AuditStateManager::AuditOutdatedReason::None:
                    ui->lblAuditStatus->setText("✔️ Audit naprakész");
                    break;
                case AuditStateManager::AuditOutdatedReason::OptimizeRun:
                    ui->lblAuditStatus->setText("⚠️ Az audit nem tükrözi a jelenlegi optimize eredményt");
                    break;
                case AuditStateManager::AuditOutdatedReason::StockChanged:
                    ui->lblAuditStatus->setText("⚠️ Az audit nem tükrözi a jelenlegi készletet");
                    break;
                case AuditStateManager::AuditOutdatedReason::LeftoverChanged:
                    ui->lblAuditStatus->setText("⚠️ Az audit nem tükrözi a hullók aktuális állapotát");
                    break;
                case AuditStateManager::AuditOutdatedReason::RelocationFinalized:
                    ui->lblAuditStatus->setText("⚠️ Az audit nem tükrözi a relocation utáni állapotot");
                    break;
                }
            });


    auto h = SettingsManager::instance().cuttingStrategy();
    if (h == Cutting::Optimizer::TargetHeuristic::ByTotalLength) {
        ui->radioByTotalLength->setChecked(true);
    } else if (h == Cutting::Optimizer::TargetHeuristic::ByCount) {
        ui->radioByCount->setChecked(true);
    } else{
        zEvent("ismeretlen cutting strategy beállítás");
    }

    int ix = SettingsManager::instance().lastActiveTab();
    ui->midBox->setCurrentIndex(ix);

    auto scNewRequest = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(scNewRequest, &QShortcut::activated, this, [this]() {
        handle_btn_AddCuttingPlanRequest_clicked();
    });

    auto scGoToCutRequest = new QShortcut(QKeySequence("Alt+1"), this);
    connect(scGoToCutRequest, &QShortcut::activated, this, [this]() {
        ui->midBox->setCurrentIndex(0); // vagy a CutRequest tab indexe
    });

    ui->chkUseLeftovers->setChecked(
        SettingsManager::instance().useReusableLeftovers()
        );

    connect(ui->chkUseLeftovers, &QCheckBox::toggled,
            [](bool checked){
                SettingsManager::instance().setUseReusableLeftovers(checked);
            });

    _seriesMatrixView = new SeriesMatrixView(this, presenter);
    _seriesMatrixView->hide();   // nem automatikusan jelenik meg
    connect(_seriesMatrixView, &SeriesMatrixView::matrixClosed,
            this, &MainWindow::onSeriesMatrixClosed);

    translate();
    zEventINFO("✅ MainWindow inited");
}


void MainWindow::translate(){
    ui->radioByCount->setToolTip("📊 Ahol több a darab – gyors feldolgozás");
    ui->radioByTotalLength->setToolTip("📏 Ahol több az anyag – jobb kihasználás");
}

void MainWindow::ButtonConnector_Connect()
{
    //cutting plan requests
    connect(ui->btn_AddCuttingPlanRequest, &QPushButton::clicked,
               this, &MainWindow::handle_btn_AddCuttingPlanRequest_clicked);
    connect(ui->btn_NewCuttingPlan, &QPushButton::clicked,
               this, &MainWindow::handle_btn_NewRequest_clicked);
    connect(ui->btn_ClearCuttingPlan, &QPushButton::clicked,
               this, &MainWindow::handle_btn_ClearRequest_clicked);

    connect(ui->btn_CloneRequest, &QPushButton::clicked,
            this, &MainWindow::handle_btn_CloneRequest_clicked);

    // stock table
    connect(ui->btn_AddStockEntry, &QPushButton::clicked,
               this, &MainWindow::handle_btn_AddStockEntry_clicked);

    // leftover table
    connect(ui->btn_AddLeftoverStockEntry, &QPushButton::clicked,
            this, &MainWindow::handle_btn_AddLeftoverStockEntry_clicked);

    connect(ui->btn_LeftoverDisposal, &QPushButton::clicked,
               this, &MainWindow::handle_btn_LeftoverDisposal_clicked);

    // Leftover Intake Form export
    connect(ui->btn_ExportLeftoverForm, &QPushButton::clicked,
            this, &MainWindow::handle_btn_ExportLeftoverForm_clicked);
    // cutting plan

    connect(ui->btn_Optimize, &QPushButton::clicked,
               this, &MainWindow::handle_btn_Optimize_clicked);

    connect(ui->btn_ExportCutPlanSummary, &QPushButton::clicked,
            this, &MainWindow::handle_btn_ExportCutPlanSummary_clicked);

    connect(ui->btn_OpenCuttingPlan, &QPushButton::clicked,
            this, &MainWindow::handle_btn_OpenRequest_clicked);

    connect(ui->btn_Finalize_2, &QPushButton::clicked,
            this, &MainWindow::handle_btn_RelocationPlanFinalize_clicked);

    connect(ui->radioByCount, &QPushButton::toggled,
     this, &MainWindow::handle_btn_OptRad_clicked);

    connect(ui->radioByTotalLength, &QPushButton::toggled,
     this, &MainWindow::handle_btn_OptRad_clicked);

    // storage audit
    connect(ui->btn_StorageAudit, &QPushButton::clicked,
            this, &MainWindow::handle_btn_StorageAudit_clicked);

    // relocation
    connect(ui->btn_Relocate, &QPushButton::clicked,
            this, &MainWindow::handle_btn_Relocate_clicked);

    // generate cutting plan
    connect(ui->btn_GenerateCutInstruction, &QPushButton::clicked,
            this, &MainWindow::handle_btn_GenerateCuttingPlan_clicked);

    connect(ui->btn_ExportCutInstruction, &QPushButton::clicked,
            this, &MainWindow::handle_btn_ExportCutInstruction_clicked);

    connect(ui->btn_Painter, &QPushButton::clicked,
            this, &MainWindow::handle_btn_Painter_clicked);

    connect(ui->btn_Audit, &QPushButton::clicked,
            this, &MainWindow::handle_btn_Audit_clicked);

    connect(ui->btn_BOMaudit, &QPushButton::clicked,
            this, &MainWindow::handle_btn_BOMaudit_clicked);

}

void MainWindow::mainToolbarBuilder(ActionConnectorModel& m)
{
    ui->mainToolBar->addAction(m.actMaterialFinder);
    ui->mainToolBar->addAction(m.actSettings);
    ui->mainToolBar->addAction(m.actSeriesMatrix);   // 🔹 ÚJ
}

void MainWindow::ActionConnector_connect(ActionConnectorModel& m)
{        
    connect(m.actMaterialFinder, &QAction::triggered,
            this, &MainWindow::handle_act_MaterialFinder_clicked);
    connect(m.actSettings, &QAction::triggered,
            this, &MainWindow::handle_act_Settings_clicked);
    connect(m.actSeriesMatrix, &QAction::toggled,
            this, [this](bool checked) {
                _seriesMatrixView->setVisible(checked);

                if (checked) {

                    // 🔍 1) összes request lekérése
                    auto all = CuttingPlanRequestRegistry::instance().readAll();

                    if (!all.isEmpty()) {

                        // 🔍 2) utolsó tételszám meghatározása
                        const auto& last = all.last();
                        QString lastRef   = last.externalReference;
                        QString lastOwner = last.ownerName;

                        // 🔥 3) automatikus sorozat betöltés
                        //_seriesMatrixView->onSeriesContextChanged(lastOwner, lastRef);
                        ActiveSeries s;
                        s.active = true;
                        s.startRef = lastRef;
                        QSet<QString> seen;
                        for (const auto& r : all) {
                            if (!seen.contains(r.externalReference)) {
                                seen.insert(r.externalReference);
                                s.order.append(r.externalReference);
                            }
                        }
                        s.currentColumnIndex = s.order.indexOf(lastRef);
                        s.currentMaterialIndex = 0;

                        // ⭐ filledCells induló feltöltése
                        for (const auto& r : all)
                            _seriesMatrixView->addFilledCell(r.externalReference.trimmed(), r.materialId);

                        _seriesMatrixView->updateMatrix(s, all);

                    }

                    _seriesMatrixView->raise();
                    _seriesMatrixView->activateWindow();
                }
            });


}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::event(QEvent* e)
{
    // 🎯 Ha ez egy LambdaEvent, akkor futtatjuk a benne levő lambdát
    if (e->type() == QEvent::User) {
        auto* lambdaEvent = static_cast<LambdaEvent*>(e);
        lambdaEvent->execute();
        return true;
    }

    // 🔄 Egyéb események átadása az alapkezelésnek
    return QMainWindow::event(e);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // 🔄 Fejléc állapot mentése
    // oszlopszélesség
    SettingsManager::instance().setInputTableHeaderState(ui->tableInput->horizontalHeader()->saveState());
    SettingsManager::instance().setResultsTableHeaderState(ui->tableResults->horizontalHeader()->saveState());
    SettingsManager::instance().setStockTableHeaderState(ui->tableStock->horizontalHeader()->saveState());
    SettingsManager::instance().setLeftoversTableHeaderState(ui->tableLeftovers->horizontalHeader()->saveState());
    SettingsManager::instance().setStorageAuditTableHeaderState(ui->tableStorageAudit->horizontalHeader()->saveState());
    SettingsManager::instance().setRelocationOrderTableHeaderState(ui->tableRelocationOrder->horizontalHeader()->saveState());
    SettingsManager::instance().setCuttingInstructionTableHeaderState(ui->tableCuttingInstruction->horizontalHeader()->saveState());

    // ablakméret
    SettingsManager::instance().setWindowGeometry(saveGeometry());
    // splitter
    SettingsManager::instance().setMainSplitterState(ui->mainSplitter->saveState());

    SettingsManager::instance().setLastActiveTab(ui->midBox->currentIndex());

    SettingsManager::instance().save();

    // ✅ Bezárás engedélyezése
    event->accept();
}

void MainWindow::setInputFileLabel(const QString& label, const QString& tooltip) {
    ui->inputFileLabel->setText(label);
    ui->inputFileLabel->setToolTip(tooltip);
}

void MainWindow::ShowWarningDialog(const QString& msg) {
    QMessageBox::warning(this, "Hiba", msg);
}

// void MainWindow::updateStats(const QVector<Cutting::Plan::CutPlan>& plans, const QVector<Cutting::Result::ResultModel>& results) {
//     //analyticsPanel->updateStats(plans, results);
//     QStringList a = generateStatsStrings(plans, results);
//     zEvent(a);
// }

/*cuttingplan*/
void MainWindow::handle_btn_NewRequest_clicked()
{
    //Q_ASSERT(false); // itt megáll a debugger
    presenter->createNew_CuttingPlanRequests();
}

void MainWindow::handle_btn_RelocationPlanFinalize_clicked()
{
    int finalizedCount = 0;

    for (const auto& rowId : relocationPlanTableManager->allRowIds()) {
        const RelocationInstruction& instr = relocationPlanTableManager->getInstruction(rowId);

        if (instr.isSummary) continue;

        if (instr.isReadyToFinalize() && !instr.isAlreadyFinalized()) {
            relocationPlanTableManager->finalizeRow(rowId);
            ++finalizedCount;
        }
    }

    refreshSummaryRows();

    // 🔹 EventLogger bejegyzés
    if (finalizedCount > 0) {
        zEvent(QStringLiteral("Totál finalize lefutott: %1 sor lezárva").arg(finalizedCount));
        presenter->auditStateManager()->setOutdated(AuditStateManager::AuditOutdatedReason::RelocationFinalized);
    } else {
        zEvent("Totál finalize lefutott: nem volt lezárható sor");
    }
}



void MainWindow::refreshSummaryRows()
{
    // Lekérjük az aktuális cutPlan + audit snapshotot
    auto cutPlans = presenter->getPlansRef();
    auto auditRows = presenter->getLastAuditRows();

    // Új tervet építünk
    auto newPlan = RelocationPlanner::buildPlan(cutPlans, auditRows);

    // Csak az összesítő sorokat frissítjük a táblában
    for (const auto& instr : newPlan) {
        if (!instr.isSummary) continue;

        relocationPlanTableManager->updateSummaryRow(instr);
    }
}




void MainWindow::handle_btn_OpenRequest_clicked()
{

    const QString folder = FileNameHelper::instance().getCuttingPlanFolder();

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Vágási terv betöltése",
        folder,
        "Vágási tervek (*.csv *.txt)"
        );

    if (filePath.isEmpty())
        return;

    // 2️⃣ Beolvasás
    if (!presenter->loadCuttingPlanFromFile(filePath)) {
        QMessageBox::warning(this, tr("Hiba"), tr("Nem sikerült betölteni a vágási tervet."));
        return;
    }

    // 3️⃣ Fájlnév mentése a settingsbe
    QString fileName = QFileInfo(filePath).fileName();
    SettingsManager::instance().setCuttingPlanFileName(fileName);
    setInputFileLabel(fileName, filePath);

    // 4️⃣ Táblázat frissítése
    inputTableManager->refresh_TableFromRegistry(); // Feltölti a tableInput-ot a CuttingPlanRequestRegistry alapján
}


void MainWindow::handle_btn_ClearRequest_clicked()
{
    presenter->removeAll_CuttingPlanRequests();
}

/*cuttingplanrequests*/
void MainWindow::handle_btn_AddCuttingPlanRequest_clicked() {

    while(true){
        AddInputDialog dialog(this, DialogMode::Create);

        connect(&dialog, &AddInputDialog::seriesContextChanged,
                this, [this, &dialog](const QString& owner, const QString& ref) {

                // // 1) teljes sorozat
                // auto full = CuttingPlanRequestRegistry::instance().readAll();

                // // 2) ActiveSeries frissítése (DE NEM updateMatrix!)
                // ActiveSeries s;
                // s.active = true;
                // s.startRef = ref;

                // // 3) teljes tételszám-lista
                // for (const auto& r : full)
                //     s.order.append(r.externalReference);

                // // 4) aktuális oszlop beállítása
                // s.currentColumnIndex = s.order.indexOf(ref);
                // s.currentMaterialIndex = 0;

                // // 5) csak az ActiveSeries-t frissítjük
                // _seriesMatrixView->setActiveSeries(s);
                });


        if (dialog.exec() != QDialog::Accepted)
            return;

        Cutting::Plan::Request request = dialog.getModel();
        presenter->add_CuttingPlanRequest(request);

        // ⭐ filledCells cache frissítése
        _seriesMatrixView->addFilledCell(request.externalReference, request.materialId);
        // ⭐ BOM cache kiütése
        _seriesMatrixView->clearBomCache();

        // ⭐ Új request bekerült → mátrixot újra kell tölteni
        _seriesMatrixView->refreshAfterAdd(request.externalReference);

        if(!dialog.shouldRepeat())
            break;
    }
}

/*stock*/
void MainWindow::handle_btn_AddStockEntry_clicked()
{
    AddStockDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    StockEntry entry = dlg.getModel();
    entry.createdAt = QDateTime::currentDateTime();
    entry.lastSeenAt = entry.createdAt;

    presenter->add_StockEntry(entry);
}

/*leftover stock*/
void MainWindow::handle_btn_AddLeftoverStockEntry_clicked() {

    while (true) {
        AddWasteDialog dialog(this);

        LeftoverStockEntry request0;
        request0.source = Cutting::Result::LeftoverSource::Manual;
        request0.availableLength_mm = 0; // alapérték, hogy ne legyen üres mező

        dialog.setModel(request0); // előfeltöltés a forrással

        if (dialog.exec() != QDialog::Accepted)
            return;

        LeftoverStockEntry request = dialog.getModel();
        request.createdAt = QDateTime::currentDateTime();
        request.lastSeenAt = request.createdAt;

        presenter->add_LeftoverStockEntry(request);

        if(!dialog.shouldRepeat())
            break;
    }
}

void MainWindow::handle_btn_LeftoverDisposal_clicked()
{
    leftoverTableManager->openScrapDialog();
    // const auto confirm = QMessageBox::question(this, "Selejtezés",
    //                                            "Biztosan eltávolítod a túl rövid reusable darabokat? Ezek archiválásra kerülnek és kikerülnek a készletből.",
    //                                            QMessageBox::Yes | QMessageBox::No);

    // if (confirm == QMessageBox::Yes) {
    //     presenter->scrapShortLeftovers(); // 🔧 Selejtezési logika átkerül Presenterbe

    //     refresh_StockTable(); // ha a reusable a készletben is megjelenik
    //     //ReusableStockRegistry::instance().all());
    //     refresh_LeftoversTable();
    //     // updateArchivedWasteTable(); → ha van külön nézet hozzá

    //     QMessageBox::information(this, "Selejtezés kész",
    //                              "A túl rövid reusable darabok selejtezése megtörtént.");
    // }

}

void MainWindow::handle_btn_Optimize_clicked() {
    // 🧠 Modell frissítése
    presenter->syncModelWithRegistries();

    // 🎛️ CuttingStrategy kiválasztása a radio gombok alapján
    Cutting::Optimizer::TargetHeuristic h = Cutting::Optimizer::TargetHeuristic::ByCount;
    if (ui->radioByTotalLength->isChecked()) {
        h = Cutting::Optimizer::TargetHeuristic::ByTotalLength;
    }

    // 🚀 Optimalizálás elindítása
    presenter->runOptimization(h);
}


void MainWindow::handle_btn_ExportCutPlanSummary_clicked() {
    presenter->ExportCutPlanSummary();
}

void MainWindow::handle_btn_OptRad_clicked(bool checked)
{
    if (!checked) return; // csak akkor reagálunk, ha most lett bekapcsolva

    Cutting::Optimizer::TargetHeuristic h =
        ui->radioByTotalLength->isChecked()
            ? Cutting::Optimizer::TargetHeuristic::ByTotalLength
            : Cutting::Optimizer::TargetHeuristic::ByCount;

    // 💾 Mentés az ini-be
    SettingsManager::instance().setCuttingStrategy(h);


    // 📝 Debug log
    zInfo(QString("Cutting strategy changed to %1")
              .arg(h == Cutting::Optimizer::TargetHeuristic::ByCount
                       ? "ByCount 📊"
                       : "ByTotalLength 📏"));
}

// input table
void MainWindow::addRow_InputTable(const Cutting::Plan::Request& v)
{
    inputTableManager->addRow(v);
}

void MainWindow::updateRow_InputTable(const Cutting::Plan::Request& v)
{
    inputTableManager->updateRow(v.requestId, v);
}

void MainWindow::removeRow_InputTable(const QUuid& id)
{
    inputTableManager->removeRowById(id);
}

void MainWindow::clear_InputTable(){
    inputTableManager->clearTable();
}

// stock table
void MainWindow::addRow_StockTable(const StockEntry& v)
{
    stockTableManager->addRow(v);
}

void MainWindow::updateRow_StockTable(const StockEntry& v)
{
    stockTableManager->updateRow(v);
}

void MainWindow::removeRow_StockTable(const QUuid& id)
{
    stockTableManager->removeRowById(id);
}

void MainWindow::refresh_StockTable(){
    stockTableManager->refresh_TableFromRegistry();
}

// leftovers table
void MainWindow::addRow_LeftoversTable(const LeftoverStockEntry& v)
{
    leftoverTableManager->addRow(v);
}

void MainWindow::updateRow_LeftoversTable(const LeftoverStockEntry& v)
{
    leftoverTableManager->updateRow(v);
}

void MainWindow::removeRow_LeftoversTable(const QUuid& id)
{
    leftoverTableManager->removeRowById(id);
}


void MainWindow::refresh_LeftoversTable(){
    leftoverTableManager->refresh_TableFromRegistry();
}


// restults table
void MainWindow::clear_ResultsTable() {
    ui->tableResults->setRowCount(0);
}

void MainWindow::update_ResultsTable(const QVector<Cutting::Plan::CutPlan>& plans) {
    ui->tableResults->clearContents();
    ui->tableResults->setRowCount(0);

    for (int i = 0; i < plans.size(); ++i) {
        const Cutting::Plan::CutPlan& plan = plans[i];
        //addRow_ResultsTable(plan.rodId, plan);
        resultsTableManager->addRow(plan); // Rod #1, Rod #2, stb.
    }

    //ui->tableResults->resizeColumnsToContents();
}

void MainWindow::handle_btn_StorageAudit_clicked()
{
    presenter->runStorageAudit();             // 🧠 Audit elindítása
}

void MainWindow::update_StorageAuditTable(const QVector<StorageAuditRow>& rows) {
    ui->tableStorageAudit->clearContents();
    ui->tableStorageAudit->setRowCount(0);

    for (int i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        storageAuditTableManager->addRow(row);     // 🧱 Sor hozzáadása
    }
}

void MainWindow::updateRow_StorageAuditTable(const StorageAuditRow& row) {
    storageAuditTableManager->updateRow(row);
}


void MainWindow::handle_btn_Relocate_clicked()
{
    // 1️⃣ Adatok összegyűjtése
    auto cutPlans = presenter->getPlansRef();
    auto auditRows = presenter->getLastAuditRows();

    // 2️⃣ Relocation terv generálása
    auto relocationPlan = RelocationPlanner::buildPlan(cutPlans, auditRows);

    // 3️⃣ Gyors lista megjelenítése (pl. max 5 sor)
    ui->relocateQuickList->setPlainText(format(relocationPlan));

    // 4️⃣ Tábla feltöltése
    relocationPlanTableManager->clearTable();
    for (const auto& instr : relocationPlan) {
        relocationPlanTableManager->addRow(instr);
    }
}


// a sourcematerial annak kéne legyen, ahol az ador row anyaga megtalálható - tehát ez egy tárhely lista
// nincs benne a material barcode - sem id
QString MainWindow::format(const QList<RelocationInstruction>& items) {
    QString out;
    out += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
               .arg("Anyag",    -24)
               .arg("Mennyiség",-12)
               .arg("Forrás",   -30)
               .arg("Cél",      -30)
               .arg("Vonalkód", -20)
               .arg("Típus",    -12);
    out += QString("-").repeated(140) + "\n";

    for (const auto& it : items) {
        if (it.isSummary) {
            // 🔹 Összesítő sor formázása
            // Most már a coveredQty-t használjuk, nem a totalRemaining+movedQty-t
            QString qtyText = QString("%1/%2 (%3 maradék + %4 odavitt)")
                                  .arg(it.coveredQty)
                                  .arg(it.plannedQuantity)
                                  .arg(it.usedFromRemaining)   // maradék
                                  .arg(it.movedQty);        // odavitt


            QString statusText = it.summaryText.isEmpty()
                                     ? QString("Összesítő sor")
                                     : it.summaryText;

            out += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
                       .arg(it.materialName, -24)
                       .arg(qtyText,        -12)
                       .arg("—",            -30)   // Forrás nem releváns
                       .arg("—",            -30)   // Cél nem releváns
                       .arg("—",            -20)   // Vonalkód nem releváns
                       .arg(QString("Σ %1").arg(statusText), -12);
        } else {
            // 🔹 Normál relocation sor
            QStringList sourceParts;
            for (const auto& src : it.sources) {
                sourceParts << QString("%1 (%2/%3)")
                .arg(src.locationName)
                    .arg(src.moved)
                    .arg(src.available);
            }
            QString sourceText = sourceParts.isEmpty() ? "—" : sourceParts.join(", ");

            QStringList targetParts;
            for (const auto& tgt : it.targets) {
                targetParts << QString("%1 (%2)")
                .arg(tgt.locationName)
                    .arg(tgt.placed);
            }
            QString targetText = targetParts.isEmpty() ? "—" : targetParts.join(", ");

            QString qtyText = it.isSatisfied
                                  ? QStringLiteral("✔ Megvan")
                                  : QString::number(it.plannedQuantity);

            QString typeText = (it.sourceType == AuditSourceType::Stock)
                                   ? "Stock"
                                   : "Hulló";

            out += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
                       .arg(it.materialName, -24)
                       .arg(qtyText,        -12)
                       .arg(sourceText,     -30)
                       .arg(targetText,     -30)
                       .arg(it.barcode,     -20)
                       .arg(typeText,       -12);
        }
    }

    return out;
}



void MainWindow::showAuditCheckbox(const QUuid& rowId)
{
   storageAuditTableManager->showAuditCheckbox(rowId);
}

// void MainWindow::handle_btn_GenerateCuttingPlan_clicked()
// {

//     //static const QString errevent = QStringLiteral("❌ CutInstructions export nem hajtható végre. Részletek a logban.");
//     //static const QString oklog    = QStringLiteral("✅ CutInstructions exportálva: %1");

//     QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

//     _machineCutsList.clear();
//     // 1️⃣ Adatok összegyűjtése a Presenterből
//     auto& cutPlans = presenter->getPlansRef();          // vágási tervek
//     auto leftovers = presenter->getLeftoverResults();   // hullók (külön kezelhetők)

//     QHash<QUuid,int> requestPieceCounters;

//     // 2️⃣ Tábla ürítése
//     cuttingInstructionTableManager->clearTable();

//     //Előfeldolgozás: reused leftoverek kigyűjtése
//     QSet<QString> reusedLeftovers;
//     for (const auto& plan : cutPlans) {
//         if (!plan.sourceBarcode.isEmpty()) {
//             reusedLeftovers.insert(plan.sourceBarcode);
//         }
//     }

//     int globalStep = 1;

//     // === FÁZIS 1: MachineCuts modell feltöltése ===
//     for (const auto& plan : cutPlans) {
//         // Gépadatok lekérése a registry-ből
//         const CuttingMachine* machine =
//             CuttingMachineRegistry::instance().findById(plan.machineId);
//         if (!machine) continue;

//         // Megnézzük, van-e már ilyen gép a listában
//         auto it = std::find_if(_machineCutsList.begin(), _machineCutsList.end(),
//                                [&](const MachineCuts& mc){ return mc.machineHeader.machineId == plan.machineId; });
//         if (it == _machineCutsList.end()) {
//             // Ha nincs, új MachineCuts blokkot hozunk létre
//             MachineCuts mc;
//             mc.machineHeader.machineId = plan.machineId;
//             mc.machineHeader.machineName = machine->name;
//             mc.machineHeader.comment = machine->comment;
//             mc.machineHeader.kerf_mm = machine->kerf_mm;
//             mc.machineHeader.stellerMaxLength_mm = machine->stellerMaxLength_mm;
//             mc.machineHeader.stellerCompensation_mm = machine->stellerCompensation_mm;
//             _machineCutsList.push_back(std::move(mc));
//             it = _machineCutsList.end() - 1;
//         }

//         // Utolsó Piece index meghatározása (leftover flaghez)
//         int lastPieceIdx = -1;
//         for (int j = plan.segments.size() - 1; j >= 0; --j) {
//             if (plan.segments[j].type() == Cutting::Segment::SegmentModel::Type::Piece) {
//                 lastPieceIdx = j;
//                 break;
//             }
//         }

//         // CutInstruction-ok előállítása
//         double remaining = plan.totalLength;
//         for (int i = 0; i < plan.segments.size(); ++i) {
//             const auto& seg = plan.segments[i];
//             if (seg.type() == Cutting::Segment::SegmentModel::Type::Piece) {
//                 CutInstruction ci;
//                 ci.globalStepId = globalStep++;
//                 ci.rodId = plan.rodId;
//                 ci.materialId = plan.materialId;
//                 ci.barcode = plan.sourceBarcode;
//                 ci.cutSize_mm = seg.length_mm();
//                 ci.kerf_mm = machine->kerf_mm;
//                 ci.lengthBefore_mm = remaining;
//                 ci.computeRemaining();
//                 ci.requestId = seg._requestId;
//                 ci.status = CutStatus::Pending;
//                 ci.leftoverBarcode = plan.leftoverBarcode;

//                 // 🔹 Új: darab azonosító kiosztása per request
//                 int count = ++requestPieceCounters[seg._requestId];
//                 ci.pieceCounter = count;

//                 // Ha ez az utolsó Piece → leftover jelölés
//                 if (i == lastPieceIdx && ci.lengthAfter_mm > 0) {
//                     if (!reusedLeftovers.contains(plan.leftoverBarcode)) {
//                         ci.isFinalLeftover = true;
//                     }
//                 }

//                 it->cutInstructions.push_back(ci);
//                 remaining = ci.lengthAfter_mm;
//             }
//         }
//     }

//     // === FÁZIS 2: Rendezés / csoportosítás ===
//     for (auto& mc : _machineCutsList) {
//         CuttingInstructionUtils::postProcessMachineCuts(mc,CuttingInstructionUtils::SortStrategy::BySizeDesc); // 🔑 utófeldolgozás
//     }

//     // === FÁZIS 3: Kirakás a táblába ===
//     renderCuttingInstructions();

//     QStringList block;
//     QString fileName = SettingsManager::instance().cuttingPlanFileName();
//     QFileInfo fi(fileName);
//     QString baseName = fi.completeBaseName();

//     //block << "🧾 Vágási utasítások:";

//     QStringList labelBlock;
//     labelBlock << "🏷️ Címketáblák:";


//     int colums = 3;
//     int w = 80;

//     for (const auto& mc : _machineCutsList) {
//         // --- Vágási utasítások ---
//         //block << QString("🪚 Gép: %1").arg(mc.machineHeader.machineName);
//         // A CutInstructions (MachineCuts) IGEN, gépenkénti
//         block << CuttingInstructionUtils::formatMachineCutsEvent(mc, baseName);
//         block << ""; // üres sor gépek között

//         // --- Címketábla külön ---
//         // A CutInstructions (MachineCuts) IGEN, gépenkénti
//         QVector<CuttingInstructionUtils::LabelModel> labels =
//             CuttingInstructionUtils::collectLabelModelsFromMachineCuts(mc);

//         labelBlock << QString("🪚 Gép: %1").arg(mc.machineHeader.machineName);
//         labelBlock << CuttingInstructionUtils::formatLabelTable4(labels,w,colums,1);
//         labelBlock << ""; // üres sor gépek között
//     }


//     // === FÁZIS 4: Fájlba írás ===



//     if (baseName.isEmpty()) {
//         zEvent("❌ Nincs Cutting Plan fájlnév — az export nem hajtható végre.");
//         return;
//     }


//     // Export mappa
//     QString dir = fi.absolutePath() + "/_reports";
//     QDir().mkpath(dir);

//     // --- 1) CutInstructions.txt (csak vágási utasítások) ---
//     {
//         QString path = dir + "/" + baseName + "_CutInstructions.txt";
//         QFile file(path);

//         if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {

//             QTextStream out(&file);
//             out.setEncoding(QStringConverter::Utf8);

//             for (const QString& line : block)
//                 out << line << "\n";

//             file.close();

//             QString native = QDir::toNativeSeparators(path);
//             zInfo(QString("📄 CutInstructions exportálva ide: %1").arg(native));
//             zEvent(QString("📄 CutInstructions exportálva ide: %1").arg(native));

//         } else {
//             zEvent(QString("❌ Nem sikerült megnyitni a CutInstructions fájlt: %1")
//                        .arg(path));
//         }
//     }

//     // --- 2) CutInstructions_Labels.txt (csak címketáblák) ---
//     {
//         QString path = dir + "/" + baseName + "_CutInstructions_Labels.txt";
//         QFile file(path);

//         if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {

//             QTextStream out(&file);
//             out.setEncoding(QStringConverter::Utf8);

//             out << QString("🏷️ Címketáblák (gépenként) - CutPlan: %1").arg(baseName);
//             out<<"\n";
//             out << QString("📅 Dátum: %1").arg(dateStr);
//             out<<"\n";
//             //out << "────────────────────────────────";
//             //out<<"\n";

//             for (const auto& mc : _machineCutsList) {

//                 out << QString("🪚 Gép: %1\n").arg(mc.machineHeader.machineName);

//                 QVector<CuttingInstructionUtils::LabelModel> labels =
//                     CuttingInstructionUtils::collectLabelModelsFromMachineCuts(mc);

//                 int cols = 3;
//                 int w = 80;

//                 out << CuttingInstructionUtils::formatLabelTable4(labels, w, cols, 1);
//                 out << "\n\n";
//             }

//             file.close();

//             QString native = QDir::toNativeSeparators(path);
//             zInfo(QString("🏷️ LabelTable exportálva ide: %1").arg(native));
//             zEvent(QString("🏷️ LabelTable exportálva ide: %1").arg(native));

//         } else {
//             zEvent(QString("❌ Nem sikerült megnyitni a LabelTable fájlt: %1")
//                        .arg(path));
//         }
//     }


// }


void MainWindow::initEventLogWidget() {
    EventLogger::instance().emitEvent = [this](const QString& line) {
        EventLogHelpers::appendColoredLineWithTimestamp(ui->eventLog, line);
    };

    QStringList recent = EventLogger::instance().loadRecentEventsFromLastStart();
    EventLogHelpers::appendLines(ui->eventLog, recent);
}

// MainWindow.cpp
void MainWindow::onRowFinalized(int rowIx) {
    _highlightDelegate->completedRows.insert(rowIx);
    _highlightDelegate->currentRowIx = cuttingInstructionTableManager->currentRowIx();
    ui->tableCuttingInstruction->viewport()->update();
}


// MainWindow.cpp
void MainWindow::onCompensationChanged(const QUuid& machineId, double newVal) {
    // 1️⃣ Megkeressük a megfelelő MachineCuts blokkot
    // for (auto& mc : _machineCutsList) {
    //     if (mc.machineHeader.machineId == machineId) {
    //         mc.machineHeader.stellerCompensation_mm = newVal;

    //         // 2️⃣ Újraszámolás
    //         CuttingInstructionUtils::postProcessMachineCuts(mc,CuttingInstructionUtils::SortStrategy::BySizeDesc); // 🔑 utófeldolgozás
    //         break;
    //     }
    // }
    // renderCuttingInstructions();
    presenter->UpdateCompensation(machineId, newVal);

}

void MainWindow::refresh_InputTableFromRegistry()
{
    inputTableManager->refresh_TableFromRegistry();
}

bool MainWindow::isChkUseLeftoversChecked()
{
    bool v = ui->chkUseLeftovers->isChecked();
    return v;
}


// MainWindow.cpp
void MainWindow::renderCuttingInstructions(const QVector<MachineCuts>& machineCutsList) {
    cuttingInstructionTableManager->clearTable();

    for (auto& mc : machineCutsList) {
        // Gépszeparátor sor
        cuttingInstructionTableManager->addMachineRow(mc.machineHeader);

        // Vágások sorai
        for (const auto& ci : mc.cutInstructions) {
            cuttingInstructionTableManager->addRow(ci);
        }
    }
}




// QStringList MainWindow::generateStatsStrings(
//     const QVector<Cutting::Plan::CutPlan>& plans,
//     const QVector<Cutting::Result::ResultModel>& leftovers)
// {
//     // 🔢 Inicializálás
//     int totalCuts          = 0;
//     int segmentCount       = 0;
//     int pieceCount         = 0;
//     int kerfCount          = 0;
//     int wasteCount         = 0;
//     int totalPieceLength   = 0;
//     int totalKerfLength    = 0;
//     int totalWasteLength   = 0;

//     // 📊 Tervek bejárása
//     for (const Cutting::Plan::CutPlan& plan : plans) {
//         totalCuts += plan.piecesWithMaterial.size();
//         segmentCount += plan.segments.size();

//         for (const Cutting::Segment::SegmentModel& s : plan.segments) {
//             switch (s.type()) {
//             case Cutting::Segment::SegmentModel::Type::Piece:
//                 pieceCount++;
//                 totalPieceLength += s.length_mm();
//                 break;
//             case Cutting::Segment::SegmentModel::Type::Kerf:
//                 kerfCount++;
//                 totalKerfLength += s.length_mm();
//                 break;
//             case Cutting::Segment::SegmentModel::Type::Waste:
//                 wasteCount++;
//                 totalWasteLength += s.length_mm();
//                 break;
//             }
//         }
//     }

//     // ♻️ Újrahasznosítható maradékok száma (min. 300mm)
//     int reusableWasteCount = std::count_if(leftovers.begin(), leftovers.end(),
//                                            [](const Cutting::Result::ResultModel& r) { return r.waste >= 300; });

//     // 🗃️ Végleges hulladékok száma
//     int finalWasteCount = std::count_if(leftovers.begin(), leftovers.end(),
//                                         [](const Cutting::Result::ResultModel& r) { return r.isFinalWaste; });

//     // 🚦 Hatékonyság
//     double efficiency = (totalPieceLength == 0) ? 0.0
//                                                 : static_cast<double>(totalPieceLength) /
//                                                       static_cast<double>(totalPieceLength + totalKerfLength + totalWasteLength) * 100.0;


//     int rodCount = plans.size();
//     int totalRawLength_mm = 0;
//     for (const auto& plan : plans) totalRawLength_mm += plan.totalLength;


//     // 📋 Stringek összeállítása
//     QStringList stats;
//     stats << QString("📝 Darabolás összefoglaló:");
//     stats << QString("📊 %6 szál (%7 m) %1 darab (%8 m), %2 kerf (%3 mm), %4 hulladék (%5 mm)")
//                  .arg(pieceCount)
//                  .arg(kerfCount)
//                  .arg(totalKerfLength)
//                  .arg(wasteCount)
//                  .arg(totalWasteLength)
//                  .arg(rodCount)
//                  .arg(QString::number(totalRawLength_mm / 1000.0, 'f', 2))
//                  .arg(QString::number(totalPieceLength / 1000.0, 'f', 2));

//     // stats << QString("📊 Darabolás:\n %6 szál (%7 m) • %1 darab (%8 mm), %2 kerf (%3 mm), %4 hulladék (%5 mm)")
//     //              .arg(pieceCount).arg(kerfCount).arg(totalKerfLength).arg(wasteCount).arg(totalWasteLength)
//     //              .arg(rodCount).arg(QString::number(totalRawLength_mm / 1000.0, 'f', 2)).arg(totalPieceLength);

//     stats << QString("📐 Szakaszok összesen: %1 (%2 darab + %3 kerf + %4 hulladék)")
//                  .arg(segmentCount).arg(pieceCount).arg(kerfCount).arg(wasteCount);

//     stats << QString("♻️ Újrahasználható: %1 db • Archivált végmaradék: %2 db")
//                  .arg(reusableWasteCount).arg(finalWasteCount);

//     stats << QString("🚦 Hatékonysági mutató: %1%")
//                  .arg(QString::number(efficiency, 'f', 1));

//     return stats;
// }

void MainWindow::handle_btn_GenerateCuttingPlan_clicked() {
    presenter->GenerateCutInstructions();
}

void MainWindow::handle_btn_ExportCutInstruction_clicked() {
    presenter->ExportCutInstructions();
}

void MainWindow::handle_btn_Painter_clicked(){
        presenter->Paint();
}

void MainWindow::handle_btn_Audit_clicked(){
    presenter->Audit();
}

void MainWindow::handle_btn_BOMaudit_clicked(){
    presenter->BOM_audit();
}


void MainWindow::switchToCuttingPlanTab()
{
    ui->midBox->setCurrentWidget(ui->midBoxPage1);
}

void MainWindow::switchToInstructionsPlanTab()
{
    ui->midBox->setCurrentWidget(ui->tab_5);
}

void MainWindow::handle_btn_CloneRequest_clicked()
{
    presenter->cloneRequestDialog();
}

void MainWindow::handle_btn_ExportLeftoverForm_clicked()
{
    //presenter->ExportLeftoverIntakeForm();
    presenter->ExportLeftoverIntakeForm_Pdf();
}

void MainWindow::handle_act_MaterialFinder_clicked()
{
    MaterialFinderDialog dlg(this);

    if (dlg.exec() != QDialog::Accepted)
        return;

    MaterialFinderInput input = dlg.getInput();

    // UI → Presenter
    stockPresenter->findMaterial(input.materialId, input.minLen, input.maxLen);
}

void MainWindow::handle_act_Settings_clicked(){
    SettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::onHighlightLeftover(const QUuid& id)
{
    ui->midBox->setCurrentIndex(TAB_LEFTOVER); // átváltjuk a tabot leftover nézet fülre
    leftoverTableManager->highlight(id);
}

void MainWindow::onHighlightStock(const QUuid& id)
{
    ui->midBox->setCurrentIndex(TAB_STOCK);
    stockTableManager->highlight(id);
}


void MainWindow::onShowNotFoundMessage(const QString& msg)
{
    QMessageBox::information(this, "Keresés", msg);
}

void MainWindow::onSeriesMatrixClosed()
{
    _actSeriesMatrix->setChecked(false);
}


