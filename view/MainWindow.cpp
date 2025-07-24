#include "MainWindow.h"
//#include "common/materialutils.h"
#include "common/grouputils.h"
#include "common/materialutils.h"
#include "ui_MainWindow.h"

#include "../presenter/CuttingPresenter.h"
#include "view/dialog/addinputdialog.h"

#include <QComboBox>
#include <QMessageBox>

#include "cutanalyticspanel.h" // vagy ahova tetted

//#include "../model/materialregistry.h"
#include "../model/stockentry.h"
#include "../model/cuttingrequest.h"
//#include "../model/cutresult.h"
#include "../model/CuttingOptimizerModel.h"

#include <model/registries/reusablestockregistry.h>
#include <model/registries/stockregistry.h>

#include <common/rowstyler.h>

#include <model/repositories/materialrepository.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    inputTableManager = std::make_unique<InputTableManager>(ui->tableInput, this);
    stockTableManager = std::make_unique<StockTableManager>(ui->tableStock, this);
    leftoverTableManager = std::make_unique<LeftoverTableManager>(ui->tableLeftovers, this);

      // 🟢 Vágási kérés tábla – fix méretek, alternáló sorok
    ui->tableInput->setColumnWidth(0, 120); // Anyag
    ui->tableInput->setColumnWidth(1, 70); // Hossz
    ui->tableInput->setColumnWidth(2, 70); // Darabszám
    ui->tableInput->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableInput->horizontalHeader()->setStretchLastSection(false);
    ui->tableInput->setAlternatingRowColors(true);

    // 🟡 tableResults – fix oszlopszélesség, jól olvasható
    ui->tableResults->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableResults->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tableResults->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableResults->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableResults->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);

    ui->tableResults->setColumnWidth(0, 80);   // Rod #
    ui->tableResults->setColumnWidth(1, 120);  // Anyag
    ui->tableResults->setColumnWidth(2, 200);  // Vágások
    ui->tableResults->setColumnWidth(3, 60);   // Kerf
    ui->tableResults->setColumnWidth(4, 60);   // Hulladék

    ui->tableResults->setAlternatingRowColors(true);
    ui->tableResults->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableResults->horizontalHeader()->setStretchLastSection(false);

    ui->tableResults->setColumnWidth(0, 150);
    ui->tableResults->setColumnWidth(1, 150);
    ui->tableResults->setColumnWidth(3, 60);
    ui->tableResults->setColumnWidth(4, 60);
    ui->tableResults->setColumnWidth(5, 40);

    ui->tableLeftovers->setColumnWidth(LeftoverTableManager::ColName, 120);
    ui->tableLeftovers->setColumnWidth(LeftoverTableManager::ColBarcode, 150);
    ui->tableLeftovers->setColumnWidth(LeftoverTableManager::ColReusableId, 80);
    ui->tableLeftovers->setColumnWidth(LeftoverTableManager::ColLength, 60);
    ui->tableLeftovers->setColumnWidth(LeftoverTableManager::ColShape, 60);
    ui->tableLeftovers->setColumnWidth(LeftoverTableManager::ColSource, 60);
    ui->tableLeftovers->setColumnWidth(LeftoverTableManager::ColReusable, 20);


    ui->tableStock->setColumnWidth(StockTableManager::ColName, 120);
    ui->tableStock->setColumnWidth(StockTableManager::ColBarcode, 120);
    ui->tableStock->setColumnWidth(StockTableManager::ColLength, 80);
    ui->tableStock->setColumnWidth(StockTableManager::ColShape, 80);
    ui->tableStock->setColumnWidth(StockTableManager::ColQuantity, 80);

    // 🖼️ Alap ablakbeállítások
    resize(1024, 600);
    setWindowTitle("CutPlanner MVP");

    presenter = new CuttingPresenter(this, this);

    // 📥 Tesztadatok betöltése
    inputTableManager->fillTestData();         // Feltölti a tableInput-ot

    //stockTableManager->initCustomTestStock(); // Feltölti a tableStock-ot tesztadattal
    stockTableManager->updateTableFromRepository();  // Frissíti a tableStock a StockRepository alapján
    leftoverTableManager->updateTableFromRepository();  // Feltölti a maradékokat tesztadatokkal

    analyticsPanel = new CutAnalyticsPanel(this);
    ui->midLayout->addWidget(analyticsPanel);
}


MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::on_btnAddRow_clicked() {
    AddInputDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted)
        return;

    CuttingRequest request;
    request.materialId = dialog.selectedMaterialId();
    request.requiredLength = dialog.length();
    request.quantity = dialog.quantity();

    if (!request.isValid()) {
        QMessageBox::warning(this, "Hibás adat", request.invalidReason());
        return;
    }

    presenter->addCutRequest(request);
    inputTableManager->addRow(request);
}


void MainWindow::on_btnOptimize_clicked() {
    // 📥 Bemenetek beolvasása a UI táblákból
    //QVector<CuttingRequest> requestList  = inputTableManager->readAll();
    //QVector<StockEntry>     stockList    = stockTableManager->readAll();
    QVector<ReusableStockEntry> leftoverList = leftoverTableManager->readAll(); // ♻️ hullók

    QVector<CuttingRequest> requestList  = inputTableManager->readAll();       // továbbra is UI-ból
    QVector<StockEntry>     stockList    = StockRegistry::instance().all();    // 🔁 repo-ból
    //QVector<CutResult>      leftoverList = LeftoverRegistry::instance().all(); // 🔁 repo-ból


    // ⚠️ Validáció
    if (requestList.isEmpty()) {
        QMessageBox::warning(this, "Hiba", "Nincs vágási igény megadva.");
        return;
    }

    if (stockList.isEmpty()) {
        QMessageBox::warning(this, "Hiba", "Nincs készlet megadva.");
        return;
    }

    // ♻️ Hullók konvertálása újrafelhasználható készletté
    QVector<ReusableStockEntry> reusableList;
    for (const ReusableStockEntry &r : leftoverList) {
        if (r.availableLength_mm >=
            300) { // ✅ csak releváns maradékokat vesszük figyelembe

            ReusableStockEntry e;
            e.materialId = r.materialId;
            e.availableLength_mm = r.availableLength_mm;
            e.source = r.source;                 // 🛠️ Forrás megőrzése
            e.optimizationId = r.optimizationId; // 🔍 Csak ha van
            e.barcode = r.barcode;               // 🧾 Egyedi azonosító

            reusableList.append(e);
        }
    }

    if (leftoverList.isEmpty()) {
        QMessageBox::warning(this, "Hiba", "Nincs hulló készlet megadva.");
        return;
    }


    // 🧠 Modell frissítése
    presenter->setCuttingRequests(requestList);
    presenter->setStockInventory(stockList);
    presenter->setReusableInventory(reusableList);

    // 🚀 Optimalizálás elindítása
    presenter->runOptimization();
}

void MainWindow::updateStats(const QVector<CutPlan>& plans, const QVector<CutResult>& results) {
    analyticsPanel->updateStats(plans, results);
}


void MainWindow::clearCutTable() {
    ui->tableResults->setRowCount(0);
}

void MainWindow::update_ResultsTable(const QVector<CutPlan>& plans) {
    ui->tableResults->clearContents();
    ui->tableResults->setRowCount(0);

    for (int i = 0; i < plans.size(); ++i) {
        const CutPlan& plan = plans[i];
        addRow_tableResults(plan.rodId, plan);
    }

    //ui->tableResults->resizeColumnsToContents();
}

void MainWindow::addRow_tableResults(QString rodNumber, const CutPlan& plan) {
    int row = ui->tableResults->rowCount();
    ui->tableResults->insertRow(row);       // metaadat sor
    ui->tableResults->insertRow(row + 1);   // badge sor


    // 1️⃣ Rod #
    auto* itemRod = new QTableWidgetItem(rodNumber);
    itemRod->setTextAlignment(Qt::AlignCenter);

    // 2️⃣ Cuts badge-ek
    QWidget* cutsWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(cutsWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // for (int cut : plan.cuts) {
    //     QString color;
    //     if (cut < 300)
    //         color = "#e74c3c";
    //     else if (cut > 2000)
    //         color = "#f39c12";
    //     else
    //         color = "#27ae60";

    //     QLabel* label = new QLabel(QString::number(cut));
    //     label->setAlignment(Qt::AlignCenter);
    //     label->setStyleSheet(QString(
    //                              "QLabel {"
    //                              " border-radius: 6px;"
    //                              " padding: 3px 8px;"
    //                              " color: white;"
    //                              " background-color: %1;"
    //                              " font-weight: bold;"
    //                              "}").arg(color));
    //     layout->addWidget(label);
    // }

    for (const Segment& s : plan.segments) {
        QString color;
        switch (s.type) {
        case SegmentType::Piece:
            color = s.length_mm < 300     ? "#e74c3c" :
                        s.length_mm > 2000    ? "#f39c12" :
                        "#27ae60"; break;
        case SegmentType::Kerf:   color = "#34495e"; break;
        case SegmentType::Waste:  color = "#bdc3c7"; break;
        }

        QLabel* label = new QLabel(s.toLabelString());
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QString(
                                 "QLabel {"
                                 " border-radius: 6px;"
                                 " padding: 3px 8px;"
                                 " color: white;"
                                 " background-color: %1;"
                                 " font-weight: bold;"
                                 "}").arg(color));
        layout->addWidget(label);

        if(s.type == SegmentType::Kerf) {
            label->setMinimumWidth(60);
            label->setMaximumWidth(60);

        } /*else if (s.type == SegmentType::Piece) {
            int baseWidth = s.length_mm / 10; // Példa: 1000 mm → 100px

            // De minimum 40px legyen, különben olvashatatlan
            int labelWidth = qMax(baseWidth, 80);

            label->setMinimumWidth(labelWidth);
            label->setMaximumWidth(labelWidth);
        }*/
    }


    // 3️⃣ Kerf + Waste
    auto* itemKerf = new QTableWidgetItem(QString::number(plan.kerfTotal));
    auto* itemWaste = new QTableWidgetItem(QString::number(plan.waste));
    itemKerf->setTextAlignment(Qt::AlignCenter);
    itemWaste->setTextAlignment(Qt::AlignCenter);

    // 4️⃣ Sorháttér waste alapján
    QColor bgColor;
    if (plan.waste <= 500)
        bgColor = QColor(144, 238, 144);
    else if (plan.waste >= 1500)
        bgColor = QColor(255, 120, 120);
    else
        bgColor = QColor(245, 245, 245);

    // for (auto* item : { itemRod, itemKerf, itemWaste }) {
    //     item->setBackground(bgColor);
    //     item->setForeground(Qt::black);
    // }

    for (int col = 0; col < ui->tableResults->columnCount(); ++col) {
        QTableWidgetItem* item = ui->tableResults->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            ui->tableResults->setItem(row, col, item);
        }
        item->setBackground(bgColor);
        item->setForeground(Qt::black);
    }

    cutsWidget->setAutoFillBackground(true);
    cutsWidget->setStyleSheet(QString("background-color: %1").arg(bgColor.name()));

    // 🏷️ Csoportnév badge
    QString groupName = GroupUtils::groupName(plan.materialId);
    QColor groupColor = GroupUtils::colorForGroup(plan.materialId);

    QLabel* groupLabel = new QLabel(groupName.isEmpty() ? "–" : groupName);
    groupLabel->setAlignment(Qt::AlignCenter);
    groupLabel->setStyleSheet(QString(
                                  "QLabel {"
                                  " background-color: %1;"
                                  " color: white;"
                                  " font-weight: bold;"
                                  " font-family: 'Segoe UI';"
                                  " border-radius: 6px;"
                                  " padding-top: 6px;"
                                  " padding-bottom: 6px;"
                                  " margin-left: 6px; margin-right: 6px;"
                                  "}").arg(groupColor.name()));

    layout->setContentsMargins(0, 4, 0, 4); // felül/alul margó

    // 5️⃣ Beillesztés a sorba
    ui->tableResults->setItem(row, 0, itemRod);
    ui->tableResults->setCellWidget(row, 1, groupLabel);
    ui->tableResults->setItem(row, 2, itemKerf);
    ui->tableResults->setItem(row, 3, itemWaste);


    if (ui->tableResults->columnCount() > 0) {
        ui->tableResults->setSpan(row + 1, 0, 1, ui->tableResults->columnCount());
        ui->tableResults->setCellWidget(row + 1, 0, cutsWidget);
    }

    const MaterialMaster* mat = MaterialRegistry::instance().findById(plan.materialId);
    RowStyler::applyResultStyle(ui->tableResults, row, mat, plan);
    QColor bg = MaterialUtils::colorForMaterial(*mat);
    RowStyler::applyBadgeBackground(cutsWidget, bg);

    // ui->tableResults->setSpan(row + 1, 0, 1, ui->tableResults->columnCount());
    // ui->tableResults->setCellWidget(row + 1, 0, cutsWidget);

    // for (int col = 0; col < ui->tableResults->columnCount(); ++col)
    //     ui->tableResults->item(row, col)->setBackground(bgColor);

    // cutsWidget->setStyleSheet(QString("background-color: %1").arg(bgColor.name()));


    // if (!ui->tableResults->item(row, 1))
    //     ui->tableResults->setItem(row, 1, new QTableWidgetItem());
    // ui->tableResults->item(row, 1)->setBackground(bgColor);

    // ui->tableResults->setCellWidget(row, 2, cutsWidget);

}



void MainWindow::on_btnFinalize_clicked()
{
    const auto confirm = QMessageBox::question(this, "Terv lezárása",
                                               "Biztosan lezárod ezt a vágási tervet? Ez módosítja a készletet.",
                                               QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::Yes) {
        presenter->finalizePlans();
        update_stockTable();
        update_leftoversTable(ReusableStockRegistry::instance().all()); // frissítjük új hullókkal
    }
}



void MainWindow::on_btnDisposal_clicked()
{
    const auto confirm = QMessageBox::question(this, "Selejtezés",
                                               "Biztosan eltávolítod a túl rövid reusable darabokat? Ezek archiválásra kerülnek és kikerülnek a készletből.",
                                               QMessageBox::Yes | QMessageBox::No);

    if (confirm == QMessageBox::Yes) {
        presenter->scrapShortLeftovers(); // 🔧 Selejtezési logika átkerül Presenterbe

        update_stockTable(); // ha a reusable a készletben is megjelenik
        update_leftoversTable(ReusableStockRegistry::instance().all());
        // updateArchivedWasteTable(); → ha van külön nézet hozzá

        QMessageBox::information(this, "Selejtezés kész",
                                 "A túl rövid reusable darabok selejtezése megtörtént.");
    }
}


