#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../presenter/CuttingPresenter.h"
#include "view/dialog/addinputdialog.h"

#include <QComboBox>
#include <QMessageBox>


#include "../model/materialregistry.h"
#include "../model/stockentry.h"
#include "../model/cuttingrequest.h"
#include "../model/cutresult.h"
#include "../model/CuttingOptimizerModel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

      // 🟢 Vágási kérés tábla – fix méretek, alternáló sorok
    ui->tableInput->setColumnWidth(0, 100); // Anyag
    ui->tableInput->setColumnWidth(1, 70); // Hossz
    ui->tableInput->setColumnWidth(2, 100); // Darabszám
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

    ui->tableLeftovers->setColumnWidth(0, 40);
    ui->tableResults->setColumnWidth(1, 150);
    ui->tableResults->setColumnWidth(3, 60);
    ui->tableResults->setColumnWidth(4, 60);
    ui->tableResults->setColumnWidth(5, 40);

    // 🖼️ Alap ablakbeállítások
    resize(1024, 600);
    setWindowTitle("CutPlanner MVP");

    presenter = new CuttingPresenter(this, this);

    // 📥 Tesztadatok betöltése
    initTestInputTable();              // Feltölti a tableInput-ot
    updateStockTableFromRegistry();    // Frissíti a tableStock a StockRepository alapján
    decorateTableStock();              // Stílus, színezés
    initTestLeftoversTable();          // Feltölti a maradékokat tesztadattal
}

MainWindow::~MainWindow()
{
    delete ui;
}

// void MainWindow::initTableInput() {
//     //ui->tableInput->setColumnCount(2);
//     //ui->tableInput->setHorizontalHeaderLabels(QStringList() << "Length (mm)" << "Quantity");
//     ui->tableInput->horizontalHeader()->setStretchLastSection(true);
// }

void MainWindow::addRowToTableInput(const CuttingRequest& request) {
    int row = ui->tableInput->rowCount();
    ui->tableInput->insertRow(row);

    auto opt = MaterialRegistry::instance().findById(request.materialId);
    const MaterialMaster* mat = opt ? &(*opt) : nullptr;

    // 🧪 Anyag neve
    QString name = mat ? mat->name : "(ismeretlen)";
    auto* itemName = new QTableWidgetItem(name);
    itemName->setTextAlignment(Qt::AlignCenter);
    ui->tableInput->setItem(row, 0, itemName);

    // 📏 Hossz
    auto* itemLength = new QTableWidgetItem(QString::number(request.requiredLength));
    itemLength->setTextAlignment(Qt::AlignCenter);
    ui->tableInput->setItem(row, 0, itemLength);

    // 🔢 Mennyiség
    auto* itemQty = new QTableWidgetItem(QString::number(request.quantity));
    itemQty->setTextAlignment(Qt::AlignCenter);
    ui->tableInput->setItem(row, 1, itemQty);

    // // 🏷️ Kategória
    // QString catStr = CategoryUtils::categoryToString(category);
    // auto* itemCat = new QTableWidgetItem(catStr);
    // itemCat->setTextAlignment(Qt::AlignCenter);
    // itemCat->setFont(QFont("Segoe UI", 9, QFont::Bold));
    // itemCat->setForeground(Qt::white);
    // ui->tableInput->setItem(row, 2, itemCat);

    // 🗑️ Törlésgomb
    QPushButton* btnDelete = new QPushButton("🗑️");
    btnDelete->setToolTip("Sor törlése");
    btnDelete->setFixedSize(28, 28);
    btnDelete->setStyleSheet("QPushButton { border: none; }");

    connect(btnDelete, &QPushButton::clicked, this, [=]() {
        int currentRow = ui->tableInput->indexAt(btnDelete->pos()).row();
        ui->tableInput->removeRow(currentRow);
    });

    ui->tableInput->setCellWidget(row, 3, btnDelete);

    // 🎨 Háttérszín minden cellára            
    QColor bg = QColor(CategoryUtils::badgeColorForCategory(mat->category)).lighter(160);
    for (int col = 0; col < ui->tableInput->columnCount(); ++col) {
        auto* item = ui->tableInput->item(row, col);
        if (!item) {
            item = new QTableWidgetItem();
            ui->tableInput->setItem(row, col, item);
        }
        item->setBackground(bg);
        item->setForeground(col == 2 ? Qt::white : Qt::black);
    }
}


void MainWindow::on_btnAddRow_clicked() {
    AddInputDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted) return;

    CuttingRequest request;
    request.materialId = dialog.selectedMaterialId();
    request.requiredLength = dialog.length();
    request.quantity = dialog.quantity();

    if (request.materialId.isNull() || request.requiredLength <= 0 || request.quantity <= 0) {
        QMessageBox::warning(this, "Hibás adat", "Hiányzó vagy érvénytelen vágási igény.");
        return;
    }

    presenter->addCutRequest(request);
    addRowToTableInput(request);
}


void MainWindow::on_btnOptimize_clicked() {
    QVector<CuttingRequest> requestList = readRequestsFromInputTable();
    QVector<ProfileStock> stockList     = readInventoryFromStockTable();

    decorateTableStock();

    // 🔁 Modell frissítés és optimalizálás
    presenter->setCuttingRequests(requestList);
    presenter->setStockInventory(stockList);

    presenter->runOptimization();
    // 📊 Táblák frissítése

    //updateLeftovers(presenter->getLeftoverResults());
    appendLeftovers(presenter->getLeftoverResults());
    updatePlanTable(presenter->getPlans());  // ✅ ez a helyes név
}


void MainWindow::clearCutTable() {
    ui->tableResults->setRowCount(0);
}


/**/

void MainWindow::initTestStockTable() {
    ui->tableStock->setRowCount(0);
    QStringList categories = { "RollerTube", "BottomBar" };

    struct Row {
        QString name;
        ProfileCategory category;
        int length;
        int quantity;
    };

    QVector<Row> rows =
        {
            {"Csőtengely Ø60", ProfileCategory::RollerTube, 6000, 5},
            {"Csőtengely Ø60", ProfileCategory::RollerTube, 7000, 10},
            {"Redőnysúly Ø12",  ProfileCategory::BottomBar, 4000, 12},
            {"Redőnysúly Ø22",  ProfileCategory::BottomBar, 3000, 10},
            {"Redőnysúly Ø22",  ProfileCategory::BottomBar, 4000, 8}
        };

    for (const auto& r : rows) {
        int row = ui->tableStock->rowCount();
        ui->tableStock->insertRow(row);

        ui->tableStock->setItem(row, 0, new QTableWidgetItem(r.name));
        QString categoryStr = CategoryUtils::categoryToString(r.category);
        ui->tableStock->setItem(row, 1, new QTableWidgetItem(categoryStr));
        ui->tableStock->setItem(row, 2, new QTableWidgetItem(QString::number(r.length)));
        ui->tableStock->setItem(row, 3, new QTableWidgetItem(QString::number(r.quantity)));

        //QComboBox* combo = new QComboBox();

        //combo->addItems(categories);
        // Kategória beállítása

        //combo->setCurrentText(categoryStr);

        //ui->tableStock->setCellWidget(row, 3, combo);
    }

    //ui->tableStock->setHorizontalHeaderLabels({ "Megnevezés", "Hossz", "Darabszám", "Típus" });
}

/*
void MainWindow::updateLeftovers(const QVector<CutResult> &results) {
    ui->tableLeftovers->setRowCount(0); // törlés, újratöltés

    for (int i = 0; i < results.size(); ++i) {
        const CutResult &res = results[i];
        int row = ui->tableLeftovers->rowCount();
        ui->tableLeftovers->insertRow(row);

        // Rod # (sorszám)
        auto *itemId = new QTableWidgetItem(QString::number(i + 1));
        itemId->setTextAlignment(Qt::AlignCenter);

        // Profile name
        auto *itemName = new QTableWidgetItem(res.profileName);

        // Length
        auto *itemLength = new QTableWidgetItem(QString::number(res.length));
        itemLength->setTextAlignment(Qt::AlignCenter);

        // Waste
        auto *itemWaste = new QTableWidgetItem(QString::number(res.waste));
        itemWaste->setTextAlignment(Qt::AlignCenter);

        // Cuts
        auto *itemCuts = new QTableWidgetItem(res.cutsAsString());

        // Reusable ✔ / ✘
        QString reuseMark = (res.waste >= 300) ? "✔" : "✘";
        auto *itemReusable = new QTableWidgetItem(reuseMark);
        itemReusable->setTextAlignment(Qt::AlignCenter);

        // Színezés: zöld ✔, piros ✘
        QColor bg = (reuseMark == "✔") ? QColor(144, 238, 144) : QColor(255, 200, 200);
        itemReusable->setBackground(bg);
        itemReusable->setForeground(Qt::black);

        // Beillesztés
        ui->tableLeftovers->setItem(row, 0, itemId);
        ui->tableLeftovers->setItem(row, 1, itemName);
        ui->tableLeftovers->setItem(row, 2, itemLength);
        ui->tableLeftovers->setItem(row, 3, itemWaste);
        ui->tableLeftovers->setItem(row, 4, itemCuts);
        ui->tableLeftovers->setItem(row, 5, itemReusable);
    }
}*/

// void MainWindow::updateLeftovers(const QVector<CutResult>& results) {
//     ui->tableLeftovers->setRowCount(0);

//     for (int i = 0; i < results.size(); ++i) {
//         addLeftoverRow(results[i], i);
//     }
// }


/**/

void MainWindow::initTestStockInventory() {
    profileInventory.clear();

    profileInventory.append({
        "Csőtengely Ø60", ProfileCategory::RollerTube, 7000, 2
    });
    profileInventory.append({
        "Csőtengely Ø60", ProfileCategory::RollerTube, 6000, 1
    });
    profileInventory.append({
        "Redőnysúly 3m",  ProfileCategory::BottomBar, 3000, 3
    });

    //updateStockTable(); // ha van rá metódus, frissíti a táblázatot
}

void MainWindow::updateStockTableFromRegistry() {
    const auto& materials = MaterialRegistry::instance().materials;

    ui->tableStock->clearContents();
    ui->tableStock->setRowCount(materials.size());

    for (int i = 0; i < materials.size(); ++i) {
        const auto& mat = materials[i];

        ui->tableStock->setItem(i, 0, new QTableWidgetItem(mat.name));
        ui->tableStock->setItem(i, 1, new QTableWidgetItem(mat.type.toString())); // ideiglenesen type = category
        ui->tableStock->setItem(i, 2, new QTableWidgetItem(QString::number(mat.stockLength_mm)));
        ui->tableStock->setItem(i, 3, new QTableWidgetItem("1"));
    }

    ui->tableStock->resizeColumnsToContents();
}

void MainWindow::initTestInputTable() {
    ui->tableInput->setRowCount(0);

    //QStringList categories = { "RollerTube", "BottomBar" };

    struct RowData {
        int length;
        int quantity;
        ProfileCategory category;
    };

    QVector<RowData> testRequests = {
        {1800, 2, ProfileCategory::RollerTube},
        {2200, 1, ProfileCategory::BottomBar},
        {2900, 1, ProfileCategory::RollerTube}
    };

    for (const auto& row : testRequests) {
            addRowToTableInput(row.length, row.quantity, row.category);
    }
}


void MainWindow::updateStockTable() {
    ui->tableStock->setRowCount(0);

    for (const ProfileStock &item : profileInventory) {
        int row = ui->tableStock->rowCount();
        ui->tableStock->insertRow(row);

        ui->tableStock->setItem(row, 0, new QTableWidgetItem(item.name));
        ui->tableStock->setItem(row, 1, new QTableWidgetItem(
                                            item.category == ProfileCategory::RollerTube ? "RollerTube" : "BottomBar"));
        ui->tableStock->setItem(row, 2, new QTableWidgetItem(QString::number(item.length)));
        ui->tableStock->setItem(row, 3, new QTableWidgetItem(QString::number(item.quantity)));
    }
}

/**/



/**/

QVector<ProfileStock> MainWindow::readInventoryFromStockTable() {
    QVector<ProfileStock> inventory;
    int rowCount = ui->tableStock->rowCount();

    for (int row = 0; row < rowCount; ++row) {
        auto* nameItem = ui->tableStock->item(row, 0);
        //QWidget* catWidget = ui->tableStock->cellWidget(row, 1); // ⬅️ combó!
        auto* catItem = ui->tableStock->item(row, 1);
        auto* lengthItem = ui->tableStock->item(row, 2);
        auto* qtyItem = ui->tableStock->item(row, 3);


        if (!nameItem || !lengthItem || !qtyItem || !catItem)
            continue;

        bool okLen = false, okQty = false;

        QString name = nameItem->text().trimmed();
        int length = lengthItem->text().toInt(&okLen);
        int quantity = qtyItem->text().toInt(&okQty);

        // QComboBox* combo = qobject_cast<QComboBox*>(catWidget);
        // if (!combo)
        //     continue;

        QString categoryStr = catItem->text().trimmed();
        ProfileCategory category = CategoryUtils::categoryFromString(categoryStr);

        if (!okLen || !okQty || name.isEmpty() || length <= 0 || quantity <= 0 || category == ProfileCategory::Unknown)
            continue;

        inventory.append({ name, category, length, quantity });
    }

    return inventory;
}


QVector<CuttingRequest> MainWindow::readRequestsFromInputTable() {
    QVector<CuttingRequest> result;

    int rowCount = ui->tableInput->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        auto* lengthItem   = ui->tableInput->item(row, 0);
        auto* quantityItem = ui->tableInput->item(row, 1);
        auto* categoryItem = ui->tableInput->item(row, 2);
        //auto* categoryWidget = ui->tableInput->cellWidget(row, 2); // combobox!

        if (!lengthItem || !quantityItem || !categoryItem)
            continue;

        bool okLen = false, okQty = false;
        int length   = lengthItem->text().toInt(&okLen);
        int quantity = quantityItem->text().toInt(&okQty);
        QString categoryStr = categoryItem->text();
        //QString categoryStr;

        // 👇 QComboBox-ra cast
        // QComboBox* combo = qobject_cast<QComboBox*>(categoryWidget);
        // if (combo)
        //     categoryStr = combo->currentText().trimmed();



        if (!okLen || !okQty || length <= 0 || quantity <= 0 || categoryStr.isEmpty())
            continue;

        ProfileCategory category = CategoryUtils::categoryFromString(categoryStr);
        if (category == ProfileCategory::Unknown)
            continue;

        result.append({ length, quantity, category });
    }

    return result;
}

void MainWindow::updatePlanTable(const QVector<CutPlan>& plans) {
    ui->tableResults->clearContents();
    ui->tableResults->setRowCount(0);

    for (int i = 0; i < plans.size(); ++i) {
        const CutPlan& plan = plans[i];
        addCutRow(i, plan);
    }

    //ui->tableResults->resizeColumnsToContents();
}

void MainWindow::addCutRow(int rodNumber, const CutPlan& plan) {
    int row = ui->tableResults->rowCount();
    ui->tableResults->insertRow(row);

    // 1️⃣ Rod #
    auto *itemRod = new QTableWidgetItem(QString::number(rodNumber));
    itemRod->setTextAlignment(Qt::AlignCenter);

    // 2️⃣ Cuts: badge-szerű QLabel-ek a cellában
    QWidget *cutsWidget = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(cutsWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    for (int cut : plan.cuts) {
        QString color;
        if (cut < 300)
            color = "#e74c3c"; // piros
        else if (cut > 2000)
            color = "#f39c12"; // narancssárga
        else
            color = "#27ae60"; // zöld

        QLabel *label = new QLabel(QString::number(cut));
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(QString(
                                 "QLabel {"
                                 "border-radius: 6px;"
                                 "padding: 3px 8px;"
                                 "color: white;"
                                 "background-color: %1;"
                                 "font-weight: bold;"
                                 "}").arg(color));
        layout->addWidget(label);
    }

    // 3️⃣ Kerf + Waste
    auto *itemKerf = new QTableWidgetItem(QString::number(plan.kerfTotal));
    auto *itemWaste = new QTableWidgetItem(QString::number(plan.waste));
    itemKerf->setTextAlignment(Qt::AlignCenter);
    itemWaste->setTextAlignment(Qt::AlignCenter);

    // 4️⃣ Sorháttér: kontraszt szín waste alapján
    QColor bgColor;
    if (plan.waste <= 500)
        bgColor = QColor(144, 238, 144); // Lime-zöld
    else if (plan.waste >= 1500)
        bgColor = QColor(255, 120, 120); // Korallpiros
    else
        bgColor = QColor(245, 245, 245); // semleges

    for (auto *item : {itemRod, itemKerf, itemWaste}) {
        item->setBackground(bgColor);
        item->setForeground(Qt::black);
    }
    cutsWidget->setAutoFillBackground(true);
    cutsWidget->setStyleSheet(QString("background-color: %1").arg(bgColor.name()));


    QString categoryStr = CategoryUtils::categoryToString(plan.category);
    QString badgeColor = CategoryUtils::badgeColorForCategory(plan.category);

    // 🏷️ Kategóriacímke QLabel-ként, badge-stílusban
    QLabel *categoryLabel = new QLabel(categoryStr);
    categoryLabel->setAlignment(Qt::AlignCenter);
    categoryLabel->setStyleSheet(QString(
                                     "QLabel {"
                                     " background-color: %1;"
                                     " color: white;"
                                     " font-weight: bold;"
                                     " font-family: 'Segoe UI';"
                                     " border-radius: 6px;"
                                     " padding: 3px 10px;"
                                     " margin-left: 6px; margin-right: 6px;"
                                     "}").arg(badgeColor));

    // 5️⃣ Beillesztés a sorba
    ui->tableResults->setItem(row, 0, itemRod);
    ui->tableResults->setCellWidget(row, 1, categoryLabel);
    //ui->tableResults->item(row, 1)->setBackground(bgColor); // ha van item — vagy: állíts rá egy dummy-t

    if (!ui->tableResults->item(row, 1))
        ui->tableResults->setItem(row, 1, new QTableWidgetItem());
    ui->tableResults->item(row, 1)->setBackground(bgColor);

    ui->tableResults->setCellWidget(row, 2, cutsWidget);
    ui->tableResults->setItem(row, 3, itemKerf);
    ui->tableResults->setItem(row, 4, itemWaste);
}


// void MainWindow::decorateTableStock() {
//     for (int row = 0; row < ui->tableStock->rowCount(); ++row) {
//         QWidget* widget = ui->tableStock->cellWidget(row, 3); // 4. oszlop: ComboBox
//         QComboBox* combo = qobject_cast<QComboBox*>(widget);

//         if (!combo)
//             continue;

//         QString categoryStr = combo->currentText().trimmed();
//         ProfileCategory cat = categoryFromString(categoryStr);
//         QString colorHex = badgeColorForCategory(cat);

//         // 🎨 Világosított háttér a táblázatba
//         QColor bg = QColor(colorHex).lighter(160);

//         // Színezzük a sor celláit
//         for (int col = 0; col < ui->tableStock->columnCount(); ++col) {
//             QTableWidgetItem* item = ui->tableStock->item(row, col);
//             if (item)
//                 item->setBackground(bg);
//         }

//         // ComboBox háttérszín is passzoljon
//         combo->setStyleSheet(QString(
//                                  "QComboBox { background-color: %1; border-radius: 4px; padding-left: 4px; }"
//                                  ).arg(bg.name()));
//     }
// }

void MainWindow::decorateTableStock() {
    const int categoryCol = 1; // pl. 4. oszlop a kategória neve

    for (int row = 0; row < ui->tableStock->rowCount(); ++row) {
        QTableWidgetItem* catItem = ui->tableStock->item(row, categoryCol);
        if (!catItem)
            continue;

        QString categoryStr = catItem->text().trimmed();
        ProfileCategory cat = CategoryUtils::categoryFromString(categoryStr);
        QColor bg = QColor(CategoryUtils::badgeColorForCategory(cat));//.lighter(160); // világosított szín

        // Minden cellát színezzünk meg
        for (int col = 0; col < ui->tableStock->columnCount(); ++col) {
            QTableWidgetItem* item = ui->tableStock->item(row, col);
            if (item)
                item->setBackground(bg);
        }
    }
}

/*
void MainWindow::initTestLeftoversTable() {
    ui->tableLeftovers->setRowCount(0);
    // ui->tableLeftovers->setColumnCount(6);
    // ui->tableLeftovers->setHorizontalHeaderLabels({
    //     "Hossz", "Típus", "Eredeti", "Felhasználva", "Maradék", "Vágások száma"
    // });

    struct LeftoverRow {
        int rod_id;
        ProfileCategory category;
        int originalLength; // ennyi volt eredetileg
        QVector<int> cuts; // ezek lettek belőle vágva
        int leftover; // ennyi maradt
    };

    QVector<LeftoverRow> testLeftovers = {
        { 1, ProfileCategory::RollerTube, 3000, {}, 2940 },
        { 2, ProfileCategory::RollerTube, 3000, {1800, 1200}, 180 },
        { 3, ProfileCategory::BottomBar,  4000, {600, 500}, 100 }
    };

    for (const auto& row : testLeftovers) {
        int used = row.originalLength - row.leftover;
        int cutCount = row.cuts.size();

        int r = ui->tableLeftovers->rowCount();
        ui->tableLeftovers->insertRow(r);

        // auto setItem = [&](int col, const QString& text, Qt::Alignment align = Qt::AlignCenter) {
        //     auto* item = new QTableWidgetItem(text);
        //     item->setTextAlignment(align);
        //     ui->tableLeftovers->setItem(r, col, item);
        // };

        QString reuseMark = (row.leftover >= 300) ? "✔" : "✘";
        auto *itemReusable = new QTableWidgetItem(reuseMark);
        itemReusable->setTextAlignment(Qt::AlignCenter);

        // Színezés: zöld ✔, piros ✘
        QColor bg2 = (reuseMark == "✔") ? QColor(144, 238, 144) : QColor(255, 200, 200);
        itemReusable->setBackground(bg2);
        itemReusable->setForeground(Qt::black);

        auto *item0 = new QTableWidgetItem(QString::number(row.rod_id));
        item0->setTextAlignment(Qt::AlignCenter);

        auto *item1 = new QTableWidgetItem(CategoryUtils::categoryToString(row.category));
        item1->setTextAlignment(Qt::AlignCenter);

        auto *item2 = new QTableWidgetItem(QString::number(row.originalLength));
        item2->setTextAlignment(Qt::AlignCenter);

        auto *item3 = new QTableWidgetItem(QString::number(row.leftover));
        item3->setTextAlignment(Qt::AlignCenter);

        auto *item4 = new QTableWidgetItem(QString::number(cutCount));
        item4->setTextAlignment(Qt::AlignCenter);

        ui->tableLeftovers->setItem(r, 0, item0);
        ui->tableLeftovers->setItem(r, 1, item1);
        ui->tableLeftovers->setItem(r, 2, item2);
        ui->tableLeftovers->setItem(r, 3, item3);
        ui->tableLeftovers->setItem(r, 4, item4);
        ui->tableLeftovers->setItem(r, 5, itemReusable);

        // 🎨 Színezés kategória alapján
        QColor bg = QColor(CategoryUtils::badgeColorForCategory(row.category));//.lighter(160);
        for (int col = 0; col < ui->tableLeftovers->columnCount()-1; ++col) {
            auto* item = ui->tableLeftovers->item(r, col);
            if (!item) {
                item = new QTableWidgetItem();
                ui->tableLeftovers->setItem(r, col, item);
            }
            item->setBackground(bg);
            item->setForeground(col == 1 ? Qt::white : Qt::black);
        }
    }
}

*/
/*
void MainWindow::addLeftoverRow(int rodId, ProfileCategory category, int originalLength, const QVector<int>& cuts, int leftover) {
    int used = originalLength - leftover;
    int cutCount = cuts.size();

    int r = ui->tableLeftovers->rowCount();
    ui->tableLeftovers->insertRow(r);

    QString reuseMark = (leftover >= 300) ? "✔" : "✘";
    auto* itemReusable = new QTableWidgetItem(reuseMark);
    itemReusable->setTextAlignment(Qt::AlignCenter);
    itemReusable->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
    itemReusable->setForeground(Qt::black);

    auto setItem = [&](int col, const QString& text) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        ui->tableLeftovers->setItem(r, col, item);
    };

    setItem(0, QString::number(rodId));
    setItem(1, CategoryUtils::categoryToString(category));
    setItem(2, QString::number(originalLength));
    setItem(3, QString::number(leftover));
    setItem(4, QString::number(cutCount));
    ui->tableLeftovers->setItem(r, 5, itemReusable);

    // 🎨 Színezés kategória alapján
    QColor bg = QColor(CategoryUtils::badgeColorForCategory(category));
    for (int col = 0; col < ui->tableLeftovers->columnCount() - 1; ++col) {
        auto* item = ui->tableLeftovers->item(r, col);
        if (!item) {
            item = new QTableWidgetItem();
            ui->tableLeftovers->setItem(r, col, item);
        }
        item->setBackground(bg);
        item->setForeground(col == 1 ? Qt::white : Qt::black);
    }
}
*/

void MainWindow::addLeftoverRow(const CutResult& res, int rodIndex) {
    int row = ui->tableLeftovers->rowCount();
    ui->tableLeftovers->insertRow(row);

    QString reuseMark = (res.waste >= 300) ? "✔" : "✘";
    auto* itemReusable = new QTableWidgetItem(reuseMark);
    itemReusable->setTextAlignment(Qt::AlignCenter);
    itemReusable->setBackground(reuseMark == "✔" ? QColor(144, 238, 144) : QColor(255, 200, 200));
    itemReusable->setForeground(Qt::black);

    auto setItem = [&](int col, const QString& text, Qt::Alignment align = Qt::AlignCenter) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(align);
        ui->tableLeftovers->setItem(row, col, item);
    };

    const int reusableCol = 4;

    setItem(0, QString::number(rodIndex + 1));          // ID
    setItem(1, CategoryUtils::categoryToString(res.category));
    setItem(2, QString::number(res.length));            // Eredeti hossz
    //setItem(3, QString::number(res.waste));             // Maradék
    setItem(3, res.cutsAsString());                     // Vágások
    ui->tableLeftovers->setItem(row, reusableCol, itemReusable);

    auto* itemSource = new QTableWidgetItem(res.sourceAsString());
    itemSource->setTextAlignment(Qt::AlignCenter);
    ui->tableLeftovers->setItem(row, 5, itemSource);

    // 🎨 Kategóriaalapú színezés, ha van hozzá szín
    //if (res.category.has_value()) {
    QColor bg = QColor(CategoryUtils::badgeColorForCategory(res.category));//.lighter(160);
        for (int col = 0; col < ui->tableLeftovers->columnCount(); ++col) {
        if (col == reusableCol)
            continue; // ne színezzük a ✔/✘ oszlopot

            auto* item = ui->tableLeftovers->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                ui->tableLeftovers->setItem(row, col, item);
            }
            item->setBackground(bg);
            item->setForeground(col == 1 ? Qt::white : Qt::black);
        }
    //}
}

void MainWindow::initTestLeftoversTable() {
    ui->tableLeftovers->setRowCount(0);
    // ui->tableLeftovers->setColumnCount(6);
    // ui->tableLeftovers->setHorizontalHeaderLabels({
    //     "ID", "Típus", "Eredeti", "Maradék", "Vágások", "Újrafelhasználható"
    // });

    QVector<CutResult> testLeftovers = {
        { 3000, {}, 2940, ProfileCategory::RollerTube , LeftoverSource::Manual , std::nullopt },
        { 3000, {1800, 1200}, 180, ProfileCategory::RollerTube, LeftoverSource::Manual , std::nullopt},
        { 4000, {600, 500}, 100, ProfileCategory::BottomBar, LeftoverSource::Manual , std::nullopt}
    };

    for (int i = 0; i < testLeftovers.size(); ++i) {
        addLeftoverRow(testLeftovers[i], i);
    }
}

/**/

void MainWindow::appendLeftovers(const QVector<CutResult>& newResults) {
    int startIndex = ui->tableLeftovers->rowCount();
    for (int i = 0; i < newResults.size(); ++i) {
        addLeftoverRow(newResults[i], startIndex + i);
    }
}
