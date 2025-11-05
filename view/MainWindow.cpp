#include "MainWindow.h"
#include "view/tablehelpers/tablerowpopulator.h"
#include "view/tableutils/highlightdelegate.h"
#include "view/tableutils/storageaudittable_connector.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QMessageBox>
//#include <QObject>
#include <QCloseEvent>
#include <QTimer>
#include <QFileDialog>
#include <QCheckBox>

//#include "cutanalyticspanel.h"
#include "model/stockentry.h"
#include "model/cutting/plan/request.h"

#include "common/filenamehelper.h"
#include "common/settingsmanager.h"
#include "view/tableutils/leftovertable_connector.h"
#include "view/tableutils/inputtable_connector.h"
#include "view/tableutils/stocktable_connector.h"
#include "common/qteventutil.h"

#include "dialog/stock/addstockdialog.h"
#include "dialog/input/addinputdialog.h"

#include "model/relocation/relocationinstruction.h"
#include "view/eventloghelpers.h"

#include <service/relocation/relocationplanner.h>

#include <model/registries/cuttingmachineregistry.h>

#include <common/eventlogger.h>
#include "view/viewmodels/cutting/rowgenerator.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initEventLogWidget();    

    ui->relocateQuickList->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    setWindowTitle("CutPlanner MVP");

    ui->tableInput->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableStock->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableLeftovers->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableStorageAudit->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableRelocationOrder->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    presenter = new CuttingPresenter(this, this);

    inputTableManager = std::make_unique<InputTableManager>(ui->tableInput, this);
    stockTableManager = std::make_unique<StockTableManager>(ui->tableStock, this);
    leftoverTableManager = std::make_unique<LeftoverTableManager>(ui->tableLeftovers, this);
    resultsTableManager = std::make_unique<ResultsTableManager>(ui->tableResults, this);
    storageAuditTableManager = std::make_unique<StorageAuditTableManager>(ui->tableStorageAudit, this);
    relocationPlanTableManager = std::make_unique<RelocationPlanTableManager>(ui->tableRelocationOrder, presenter, this);
    cuttingInstructionTableManager = std::make_unique<CuttingInstructionTableManager>(ui->tableCuttingInstruction, this);

    // 🔦 Sorvezető delegate bekötése
    auto* highlightDelegate = new HighlightDelegate(ui->tableCuttingInstruction);
    highlightDelegate->currentRowIx =
        cuttingInstructionTableManager->currentRowIx();
    ui->tableCuttingInstruction->setItemDelegate(highlightDelegate);

    connect(cuttingInstructionTableManager.get(), &CuttingInstructionTableManager::rowFinalized,
            this, [this, highlightDelegate](int rowIx) {
                highlightDelegate->completedRows.insert(rowIx);
                highlightDelegate->currentRowIx = cuttingInstructionTableManager->currentRowIx();
                ui->tableCuttingInstruction->viewport()->update();
            });



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

    analyticsPanel = new CutAnalyticsPanel(this);
    ui->midLayout->addWidget(analyticsPanel);

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

    ui->tableLeftovers->setColumnHidden(LeftoverTableManager::ColBarcode, true);
    ui->tableLeftovers->setColumnHidden(LeftoverTableManager::ColShape, true);

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

    // stock table
    connect(ui->btn_AddStockEntry, &QPushButton::clicked,
               this, &MainWindow::handle_btn_AddStockEntry_clicked);

    // leftover table
    connect(ui->btn_AddLeftoverStockEntry, &QPushButton::clicked,
            this, &MainWindow::handle_btn_AddLeftoverStockEntry_clicked);

    connect(ui->btn_LeftoverDisposal, &QPushButton::clicked,
               this, &MainWindow::handle_btn_LeftoverDisposal_clicked);

    // cutting plan

    connect(ui->btn_Optimize, &QPushButton::clicked,
               this, &MainWindow::handle_btn_Optimize_clicked);
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
    connect(ui->btn_GenerateCuttingPlan, &QPushButton::clicked,
            this, &MainWindow::handle_btn_GenerateCuttingPlan_clicked);
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

void MainWindow::updateStats(const QVector<Cutting::Plan::CutPlan>& plans, const QVector<Cutting::Result::ResultModel>& results) {
    analyticsPanel->updateStats(plans, results);
}

/*cuttingplan*/
void MainWindow::handle_btn_NewRequest_clicked()
{
    Q_ASSERT(false); // itt megáll a debugger
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
    AddInputDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    Cutting::Plan::Request request = dialog.getModel();
    presenter->add_CuttingPlanRequest(request);
}

/*stock*/
void MainWindow::handle_btn_AddStockEntry_clicked()
{
    AddStockDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    StockEntry entry = dlg.getModel();
    presenter->add_StockEntry(entry);
}

/*leftover stock*/
void MainWindow::handle_btn_AddLeftoverStockEntry_clicked() {
    AddWasteDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    LeftoverStockEntry request = dialog.getModel();
    presenter->add_LeftoverStockEntry(request);
}

void MainWindow::handle_btn_LeftoverDisposal_clicked()
{
    const auto confirm = QMessageBox::question(this, "Selejtezés",
                                               "Biztosan eltávolítod a túl rövid reusable darabokat? Ezek archiválásra kerülnek és kikerülnek a készletből.",
                                               QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::Yes) {
        presenter->scrapShortLeftovers(); // 🔧 Selejtezési logika átkerül Presenterbe

        refresh_StockTable(); // ha a reusable a készletben is megjelenik
        //ReusableStockRegistry::instance().all());
        refresh_LeftoversTable();
        // updateArchivedWasteTable(); → ha van külön nézet hozzá

        QMessageBox::information(this, "Selejtezés kész",
                                 "A túl rövid reusable darabok selejtezése megtörtént.");
    }
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
    inputTableManager->updateRow(v);
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
    stockTableManager->removeRowById(id);
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
        resultsTableManager->addRow(plan.rodId, plan); // Rod #1, Rod #2, stb.
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

void MainWindow::handle_btn_GenerateCuttingPlan_clicked()
{
    // 1️⃣ Adatok összegyűjtése a Presenterből
    auto& cutPlans = presenter->getPlansRef();          // vágási tervek
    auto leftovers = presenter->getLeftoverResults();   // hullók (külön kezelhetők)

    // 2️⃣ Tábla ürítése
    cuttingInstructionTableManager->clearTable();

    //Előfeldolgozás: reused leftoverek kigyűjtése
    QSet<QString> reusedLeftovers;
    for (const auto& plan : cutPlans) {
        if (!plan.sourceBarcode.isEmpty()) {
            reusedLeftovers.insert(plan.sourceBarcode);
        }
    }

    int globalStep = 1;
    QVector<MachineCuts> machineCutsList;

    // === FÁZIS 1: MachineCuts modell feltöltése ===
    for (const auto& plan : cutPlans) {
        // Gépadatok lekérése a registry-ből
        const CuttingMachine* machine =
            CuttingMachineRegistry::instance().findById(plan.machineId);
        if (!machine) continue;

        // Megnézzük, van-e már ilyen gép a listában
        auto it = std::find_if(machineCutsList.begin(), machineCutsList.end(),
                               [&](const MachineCuts& mc){ return mc.machineHeader.machineId == plan.machineId; });
        if (it == machineCutsList.end()) {
            // Ha nincs, új MachineCuts blokkot hozunk létre
            MachineCuts mc;
            mc.machineHeader.machineId = plan.machineId;
            mc.machineHeader.machineName = machine->name;
            mc.machineHeader.comment = machine->comment;
            mc.machineHeader.kerf_mm = machine->kerf_mm;
            mc.machineHeader.stellerMaxLength_mm = machine->stellerMaxLength_mm;
            mc.machineHeader.stellerCompensation_mm = machine->stellerCompensation_mm;
            machineCutsList.push_back(std::move(mc));
            it = machineCutsList.end() - 1;
        }

        // Utolsó Piece index meghatározása (leftover flaghez)
        int lastPieceIdx = -1;
        for (int j = plan.segments.size() - 1; j >= 0; --j) {
            if (plan.segments[j].type() == Cutting::Segment::SegmentModel::Type::Piece) {
                lastPieceIdx = j;
                break;
            }
        }

        // CutInstruction-ok előállítása
        double remaining = plan.totalLength;
        for (int i = 0; i < plan.segments.size(); ++i) {
            const auto& seg = plan.segments[i];
            if (seg.type() == Cutting::Segment::SegmentModel::Type::Piece) {
                CutInstruction ci;
                ci.globalStepId = globalStep++;
                ci.rodId = plan.rodId;
                ci.materialId = plan.materialId;
                ci.barcode = plan.sourceBarcode;
                ci.cutSize_mm = seg.length_mm();
                ci.kerf_mm = machine->kerf_mm;
                ci.lengthBefore_mm = remaining;
                ci.computeRemaining();
                ci.machineId = plan.machineId;
                ci.machineName = plan.machineName;
                ci.status = CutStatus::Pending;
                ci.leftoverBarcode = plan.leftoverBarcode;

                // Ha ez az utolsó Piece → leftover jelölés
                if (i == lastPieceIdx && ci.lengthAfter_mm > 0) {
                    if (!reusedLeftovers.contains(plan.leftoverBarcode)) {
                        ci.isFinalLeftover = true;
                    }
                }

                it->cutInstructions.push_back(ci);
                remaining = ci.lengthAfter_mm;
            }
        }
    }

    // === FÁZIS 2: Rendezés / csoportosítás ===
    for (auto& mc : machineCutsList) {
        // Alapértelmezett: méret szerint csökkenő sorrend
        std::sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
                  [](const CutInstruction& a, const CutInstruction& b){
                      return a.cutSize_mm > b.cutSize_mm;
                  });

        // Ha anyag szerinti csoportosítás be van kapcsolva (pl. egy checkbox alapján)
        if (false){//ui->chkGroupByMaterial->isChecked()) {
            // Itt lehetne pl. stable_sort materialId szerint, majd azon belül méret szerint
            std::stable_sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
                             [](const CutInstruction& a, const CutInstruction& b){
                                 if (a.materialId == b.materialId)
                                     return a.cutSize_mm > b.cutSize_mm;
                                 return a.materialId.toString() < b.materialId.toString();
                             });
        }
    }

    // === FÁZIS 3: Kirakás a táblába ===
    for (auto& mc : machineCutsList) {
        // Gépszeparátor sor
        cuttingInstructionTableManager->addMachineRow(mc.machineHeader);

        // Vágások sorai
        for (const auto& ci : mc.cutInstructions) {
            cuttingInstructionTableManager->addRow(ci);
        }
    }
}


// void MainWindow::handle_btn_GenerateCuttingPlan_clicked()
// {
//     // 1️⃣ Adatok összegyűjtése a Presenterből
//     auto& cutPlans = presenter->getPlansRef();          // vágási tervek
//     auto leftovers = presenter->getLeftoverResults();   // hullók

//     // 2️⃣ Tábla ürítése
//     cuttingInstructionTableManager->clearTable();

//     // 3️⃣ Gépenként szeparátor + utasítások
//     QString currentMachine;

//     // 🔧 Átmeneti workaround: mindig a "CM2" gépet használjuk
//     // const CuttingMachine* fixedMachine =
//     //     CuttingMachineRegistry::instance().findByBarcode("CM2");

//     // if (!fixedMachine) {
//     //     qWarning() << "⚠️ A 'CM2' gép nincs regisztrálva a CuttingMachineRegistry-ben!";
//     //     return;
//     // }
//     int globalStep = 1;
//     QVector<MachineCuts> machineCutsList;

//     // 1️⃣ Nyers adatokból MachineCuts feltöltése
//     for (const auto& plan : cutPlans) {
//         const CuttingMachine* machine =
//             CuttingMachineRegistry::instance().findById(plan.machineId);
//         if (!machine) continue;

//         auto it = std::find_if(machineCutsList.begin(), machineCutsList.end(),
//                                [&](const MachineCuts& mc){ return mc.machineHeader.machineId == plan.machineId; });
//         if (it == machineCutsList.end()) {
//             MachineCuts mc;
//             mc.machineHeader.machineId = plan.machineId;
//             mc.machineHeader.machineName = machine->name;
//             mc.machineHeader.comment = machine->comment;
//             mc.machineHeader.kerf_mm = machine->kerf_mm;
//             mc.machineHeader.stellerMaxLength_mm = machine->stellerMaxLength_mm;
//             mc.machineHeader.stellerCompensation_mm = machine->stellerCompensation_mm;
//             machineCutsList.push_back(std::move(mc));
//             it = machineCutsList.end() - 1;
//         }

//         int lastPieceIdx = -1;
//         for (int j = plan.segments.size() - 1; j >= 0; --j) {
//             if (plan.segments[j].type() == Cutting::Segment::SegmentModel::Type::Piece) {
//                 lastPieceIdx = j;
//                 break;
//             }
//         }

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
//                 ci.machineId = plan.machineId;
//                 ci.machineName = plan.machineName;
//                 ci.status = CutStatus::Pending;
//                 ci.leftoverBarcode = plan.leftoverBarcode;
//                 if (i == lastPieceIdx && ci.lengthAfter_mm > 0) {
//                     ci.isFinalLeftover = true;
//                 }

//                 it->cutInstructions.push_back(ci);
//                 remaining = ci.lengthAfter_mm;
//             }
//         }
//     }

//     // 2️⃣ Rendezés gépenként
//     for (auto& mc : machineCutsList) {
//         std::sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
//                   [](const CutInstruction& a, const CutInstruction& b){
//                       return a.cutSize_mm > b.cutSize_mm;
//                   });
//     }

//     // 3️⃣ Kirakás a táblába
//     for (auto& mc : machineCutsList) {
//         cuttingInstructionTableManager->addMachineRow(mc.machineHeader);

//         for (const auto& ci : mc.cutInstructions) {
//             cuttingInstructionTableManager->addRow(ci);
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



