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

    //QVector<ProfileStock> readInventoryFromStockTable();
    QVector<CuttingRequest> readRequestsFromInputTable();

    void updatePlanTable(const QVector<CutPlan> &plans);
    void appendLeftovers(const QVector<CutResult> &newResults);
    void updateStockTableFromRegistry();

private slots:
    void on_btnAddRow_clicked();
    void on_btnOptimize_clicked();

private:
    Ui::MainWindow *ui;
    CuttingPresenter* presenter;   

    void initTestInputTable();
    void initTestStockInventory();
    void initTestLeftoverResults();

    void updateStockTable();
    void decorateTableStock();

    void addRowToTableInput(const CuttingRequest& request);
    void addLeftoverRow(const CutResult& res, int rodIndex);
};
#endif // MAINWINDOW_H
