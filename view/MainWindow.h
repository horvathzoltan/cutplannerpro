#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
//#include "../model/cuttingrequest.h"
#include "../model/cutresult.h"
//#include "../model/stockentry.h"
#include "../model/CuttingOptimizerModel.h"

#include "view/managers/inputtablemanager.h"
#include "view/managers/leftovertablemanager.h"
#include "view/managers/stocktablemanager.h"
//#include "../model/materialregistry.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class CuttingPresenter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void clearCutTable();
    void addRow_tableResults(int rodNumber, const CutPlan& plan);

    void update_ResultsTable(const QVector<CutPlan> &plans);
    void update_leftoversTable(const QVector<ReusableStockEntry> &newResults){
        leftoverTableManager->appendRows(newResults);
    }

    void update_stockTable(){
        stockTableManager->updateTableFromRepository();
    }


private slots:
    void on_btnAddRow_clicked();
    void on_btnOptimize_clicked();

private:
    Ui::MainWindow *ui;
    CuttingPresenter* presenter;

    // input tábla - vágási terv
    std::unique_ptr<InputTableManager> inputTableManager;
    // stock tábla - anyagkészlet - alapanyag raktárkészlet
    std::unique_ptr<StockTableManager> stockTableManager;
    // ♻️ Hullókezelő
    std::unique_ptr<LeftoverTableManager> leftoverTableManager;
};
#endif // MAINWINDOW_H
