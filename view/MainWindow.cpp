#include "MainWindow.h"

#include "ui_MainWindow.h"

#include <QComboBox>
#include <QMessageBox>

//#include "cutanalyticspanel.h"
#include "model/stockentry.h"
#include "model/cuttingplanrequest.h"
#include "common/filenamehelper.h"
#include "common/settingsmanager.h"
#include "common/tableconnectionhelper.h"

#include <QObject>      // connect() miatt
#include <QCloseEvent>
#include <QTimer>
#include "common/qteventutil.h"
#include "dialog/addstockdialog.h"
#include "dialog/addinputdialog.h"
#include <QFileDialog>

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

void MainWindow::updateStats(const QVector<CutPlan>& plans, const QVector<CutResult>& results) {
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

    CuttingPlanRequest request = dialog.getModel();
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
void MainWindow::addRow_InputTable(const CuttingPlanRequest& v)
{
    inputTableManager->addRow(v);
}

void MainWindow::updateRow_InputTable(const CuttingPlanRequest& v)
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

void MainWindow::update_ResultsTable(const QVector<CutPlan>& plans) {
    ui->tableResults->clearContents();
    ui->tableResults->setRowCount(0);

    for (int i = 0; i < plans.size(); ++i) {
        const CutPlan& plan = plans[i];
        //addRow_ResultsTable(plan.rodId, plan);
        resultsTableManager->addRow(plan.rodId, plan); // Rod #1, Rod #2, stb.
    }

    //ui->tableResults->resizeColumnsToContents();
}
// void MainWindow::addRow_ResultsTable(QString rodNumber, const CutPlan& plan) {
//     int row = ui->tableResults->rowCount();
//     ui->tableResults->insertRow(row);       // metaadat sor
//     ui->tableResults->insertRow(row + 1);   // badge sor


//     // 1️⃣ Rod #
//     auto* itemRod = new QTableWidgetItem(rodNumber);
//     itemRod->setTextAlignment(Qt::AlignCenter);

//     // 2️⃣ Cuts badge-ek
//     QWidget* cutsWidget = new QWidget;
//     QHBoxLayout* layout = new QHBoxLayout(cutsWidget);
//     layout->setContentsMargins(0, 0, 0, 0);
//     layout->setSpacing(6);

//     // for (int cut : plan.cuts) {
//     //     QString color;
//     //     if (cut < 300)
//     //         color = "#e74c3c";
//     //     else if (cut > 2000)
//     //         color = "#f39c12";
//     //     else
//     //         color = "#27ae60";

//     //     QLabel* label = new QLabel(QString::number(cut));
//     //     label->setAlignment(Qt::AlignCenter);
//     //     label->setStyleSheet(QString(
//     //                              "QLabel {"
//     //                              " border-radius: 6px;"
//     //                              " padding: 3px 8px;"
//     //                              " color: white;"
//     //                              " background-color: %1;"
//     //                              " font-weight: bold;"
//     //                              "}").arg(color));
//     //     layout->addWidget(label);
//     // }

//     for (const Segment& s : plan.segments) {
//         QString color;
//         switch (s.type) {
//         case Segment::Type::Piece:
//             color = s.length_mm < 300     ? "#e74c3c" :
//                         s.length_mm > 2000    ? "#f39c12" :
//                         "#27ae60"; break;
//         case Segment::Type::Kerf:   color = "#34495e"; break;
//         case Segment::Type::Waste:  color = "#bdc3c7"; break;
//         }

//         QLabel* label = new QLabel(s.toLabelString());
//         label->setAlignment(Qt::AlignCenter);
//         label->setStyleSheet(QString(
//                                  "QLabel {"
//                                  " border-radius: 6px;"
//                                  " padding: 3px 8px;"
//                                  " color: white;"
//                                  " background-color: %1;"
//                                  " font-weight: bold;"
//                                  "}").arg(color));
//         layout->addWidget(label);

//         if(s.type == Segment::Type::Kerf) {
//             label->setMinimumWidth(60);
//             label->setMaximumWidth(60);

//         } /*else if (s.type == SegmentType::Piece) {
//             int baseWidth = s.length_mm / 10; // Példa: 1000 mm → 100px

//             // De minimum 40px legyen, különben olvashatatlan
//             int labelWidth = qMax(baseWidth, 80);

//             label->setMinimumWidth(labelWidth);
//             label->setMaximumWidth(labelWidth);
//         }*/
//     }


//     // 3️⃣ Kerf + Waste
//     auto* itemKerf = new QTableWidgetItem(QString::number(plan.kerfTotal));
//     auto* itemWaste = new QTableWidgetItem(QString::number(plan.waste));
//     itemKerf->setTextAlignment(Qt::AlignCenter);
//     itemWaste->setTextAlignment(Qt::AlignCenter);

//     // 4️⃣ Sorháttér waste alapján
//     QColor bgColor;
//     if (plan.waste <= 500)
//         bgColor = QColor(144, 238, 144);
//     else if (plan.waste >= 1500)
//         bgColor = QColor(255, 120, 120);
//     else
//         bgColor = QColor(245, 245, 245);

//     // for (auto* item : { itemRod, itemKerf, itemWaste }) {
//     //     item->setBackground(bgColor);
//     //     item->setForeground(Qt::black);
//     // }

//     for (int col = 0; col < ui->tableResults->columnCount(); ++col) {
//         QTableWidgetItem* item = ui->tableResults->item(row, col);
//         if (!item) {
//             item = new QTableWidgetItem();
//             ui->tableResults->setItem(row, col, item);
//         }
//         item->setBackground(bgColor);
//         item->setForeground(Qt::black);
//     }

//     cutsWidget->setAutoFillBackground(true);
//     cutsWidget->setStyleSheet(QString("background-color: %1").arg(bgColor.name()));

//     // 🏷️ Csoportnév badge
//     QString groupName = GroupUtils::groupName(plan.materialId);
//     QColor groupColor = GroupUtils::colorForGroup(plan.materialId);

//     QLabel* groupLabel = new QLabel(groupName.isEmpty() ? "–" : groupName);
//     groupLabel->setAlignment(Qt::AlignCenter);
//     groupLabel->setStyleSheet(QString(
//                                   "QLabel {"
//                                   " background-color: %1;"
//                                   " color: white;"
//                                   " font-weight: bold;"
//                                   " font-family: 'Segoe UI';"
//                                   " border-radius: 6px;"
//                                   " padding-top: 6px;"
//                                   " padding-bottom: 6px;"
//                                   " margin-left: 6px; margin-right: 6px;"
//                                   "}").arg(groupColor.name()));

//     layout->setContentsMargins(0, 4, 0, 4); // felül/alul margó

//     // 5️⃣ Beillesztés a sorba
//     ui->tableResults->setItem(row, 0, itemRod);
//     ui->tableResults->setCellWidget(row, 1, groupLabel);
//     ui->tableResults->setItem(row, 2, itemKerf);
//     ui->tableResults->setItem(row, 3, itemWaste);

//     if (ui->tableResults->columnCount() > 0) {
//         ui->tableResults->setSpan(row + 1, 0, 1, ui->tableResults->columnCount());
//         ui->tableResults->setCellWidget(row + 1, 0, cutsWidget);
//     }

//     const MaterialMaster* mat = MaterialRegistry::instance().findById(plan.materialId);
//     RowStyler::applyResultStyle(ui->tableResults, row, mat, plan);
//     QColor bg = MaterialUtils::colorForMaterial(*mat);
//     RowStyler::applyBadgeBackground(cutsWidget, bg);

//     // ui->tableResults->setSpan(row + 1, 0, 1, ui->tableResults->columnCount());
//     // ui->tableResults->setCellWidget(row + 1, 0, cutsWidget);

//     // for (int col = 0; col < ui->tableResults->columnCount(); ++col)
//     //     ui->tableResults->item(row, col)->setBackground(bgColor);

//     // cutsWidget->setStyleSheet(QString("background-color: %1").arg(bgColor.name()));


//     // if (!ui->tableResults->item(row, 1))
//     //     ui->tableResults->setItem(row, 1, new QTableWidgetItem());
//     // ui->tableResults->item(row, 1)->setBackground(bgColor);

//     // ui->tableResults->setCellWidget(row, 2, cutsWidget);

// }




