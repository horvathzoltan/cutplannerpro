#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../model/cuttingrequest.h"
#include "../model/cutresult.h"
#include "../model/stockentry.h"
#include "../model/CuttingOptimizerModel.h"
#include "../model/materialregistry.h"

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
    void addCutRow(int rodNumber, const CutPlan& plan);

    QVector<StockEntry> readInventoryFromStockTable();
    //QVector<CuttingRequest> readRequestsFromInputTable();

    void updatePlanTable(const QVector<CutPlan> &plans);
    void appendLeftovers(const QVector<CutResult> &newResults);
    void updateStockTableFromRegistry();

private slots:
    void on_btnAddRow_clicked();
    void on_btnOptimize_clicked();

private:
    Ui::MainWindow *ui;
    CuttingPresenter* presenter;

    void fillTestData_inputTable();
    void initTestStockTable();
    void initTestLeftoversTable();

    void updateStockTable();
    void decorateTableStock();

    void addRow_inputTable(const CuttingRequest& request);
    QVector<CuttingRequest> readDataFrom_inputTable();

    void addRow_tableLeftovers(const CutResult& res, int rodIndex);
    void applyVisualStyleToInputRow(int row, const MaterialMaster *mat, const CuttingRequest &request);
};
#endif // MAINWINDOW_H
