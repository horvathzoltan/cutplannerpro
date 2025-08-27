#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QComboBox>
#include <QMessageBox>
//#include <QObject>
#include <QCloseEvent>
#include <QTimer>
#include <QFileDialog>

//#include "cutanalyticspanel.h"
#include "model/stockentry.h"
#include "model/cutting/plan/request.h"

#include "common/filenamehelper.h"
#include "common/settingsmanager.h"
#include "common/tableutils/leftovertable_connector.h"
#include "common/tableutils/inputtable_connector.h"
#include "common/tableutils/stocktable_connector.h"
#include "common/qteventutil.h"

#include "dialog/stock/addstockdialog.h"
#include "dialog/addinputdialog.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    setWindowTitle("CutPlanner MVP");

    ui->tableInput->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableStock->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableLeftovers->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    presenter = new CuttingPresenter(this, this);

    inputTableManager = std::make_unique<InputTableManager>(ui->tableInput, this);
    stockTableManager = std::make_unique<StockTableManager>(ui->tableStock, this);
    leftoverTableManager = std::make_unique<LeftoverTableManager>(ui->tableLeftovers, this);
    resultsTableManager = std::make_unique<ResultsTableManager>(ui->tableResults, this);
    storageAuditTableManager = std::make_unique<StorageAuditTableManager>(ui->tableStorageAudit, this);

    InputTableConnector::Connect(this, inputTableManager.get(), presenter);
    StockTableConnector::Connect(this, stockTableManager.get(), presenter);
    LeftoverTableConnector::Connect(this, leftoverTableManager.get(), presenter);
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

    // splitter
    ui->mainSplitter->restoreState(SettingsManager::instance().mainSplitterState());

    // ablakméret - az esemény időzítve (Qt event queue-ban)
    QtEventUtil::post(this, [this]() {
        restoreGeometry(SettingsManager::instance().windowGeometry());
        ui->mainSplitter->restoreState(SettingsManager::instance().mainSplitterState());
    });

    ui->tableLeftovers->setColumnHidden(LeftoverTableManager::ColBarcode, true);
    ui->tableLeftovers->setColumnHidden(LeftoverTableManager::ColShape, true);
}

void MainWindow::ButtonConnector_Connect()
{
    //cutting plan requests
    connect(ui->btn_AddCuttingPlanRequest, &QPushButton::clicked,
               this, &MainWindow::handle_btn_AddCuttingPlanRequest_clicked);
    connect(ui->btn_NewCuttingPlan, &QPushButton::clicked,
               this, &MainWindow::handle_btn_NewCuttingPlan_clicked);
    connect(ui->btn_ClearCuttingPlan, &QPushButton::clicked,
               this, &MainWindow::handle_btn_ClearCuttingPlan_clicked);

    // stock table
    connect(ui->btn_AddStockEntry, &QPushButton::clicked,
               this, &MainWindow::handle_btn_AddStockEntry_clicked);

    // leftover table
    connect(ui->btn_AddLeftoverStockEntry, &QPushButton::clicked,
            this, &MainWindow::handle_btn_AddLeftoverStockEntry_clicked);

    connect(ui->btn_LeftoverDisposal, &QPushButton::clicked,
               this, &MainWindow::handle_btn_LeftoverDisposal_clicked);

    // cutting plan
    connect(ui->btn_Finalize, &QPushButton::clicked,
               this, &MainWindow::handle_btn_Finalize_clicked);
    connect(ui->btn_Optimize, &QPushButton::clicked,
               this, &MainWindow::handle_btn_Optimize_clicked);
    connect(ui->btn_OpenCuttingPlan, &QPushButton::clicked,
            this, &MainWindow::handle_btn_OpenCuttingPlan_clicked);
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
void MainWindow::handle_btn_NewCuttingPlan_clicked()
{
    presenter->createNew_CuttingPlanRequests();
}

void MainWindow::handle_btn_OpenCuttingPlan_clicked()
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


void MainWindow::handle_btn_ClearCuttingPlan_clicked()
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
    // 🚀 Optimalizálás elindítása
    presenter->runOptimization();
}

void MainWindow::handle_btn_Finalize_clicked()
{
    const auto confirm = QMessageBox::question(this, "Terv lezárása",
                                               "Biztosan lezárod ezt a vágási tervet? Ez módosítja a készletet.",
                                               QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::Yes) {
        presenter->finalizePlans();
        refresh_StockTable();
        //ReusableStockRegistry::instance().all());
        refresh_LeftoversTable();// frissítjük új hullókkal
    }
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

void MainWindow::on_btn_StorageAudit_clicked()
{
    QMap<QString, int> pickingmap;
    presenter->runStorageAudit(pickingmap);             // 🧠 Audit elindítása
}

void MainWindow::update_StorageAuditTable(const QVector<StorageAuditRow>& rows) {
    ui->tableStorageAudit->clearContents();
    ui->tableStorageAudit->setRowCount(0);

    for (int i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        storageAuditTableManager->addRow(row);     // 🧱 Sor hozzáadása
    }
}
