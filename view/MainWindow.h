#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
//#include "../model/cuttingrequest.h"
#include "../model/cutresult.h"
//#include "../model/stockentry.h"
//#include "../model/CuttingOptimizerModel.h"

#include "view/managers/inputtablemanager.h"
#include "view/managers/leftovertablemanager.h"
#include "view/managers/resultstablemanager.h"
#include "view/managers/stocktablemanager.h"

#include "cutanalyticspanel.h"
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

    void updateStats(const QVector<CutPlan> &plans, const QVector<CutResult> &results);
    void setInputFileLabel(const QString &label, const QString &tooltip);
    void ShowWarningDialog(const QString &msg);

    // input table
    void addRow_InputTable(const CuttingPlanRequest &v);
    void updateRow_InputTable(const CuttingPlanRequest &v);
    void removeRow_InputTable(const QUuid &v);
    void clear_InputTable();
    // stock table
    void removeRow_StockTable(const QUuid &id);
    void updateRow_StockTable(const StockEntry &v);
    void update_StockTable();
    // leftovers table
    void removeRow_LeftoversTable(const QUuid &id);
    void updateRow_LeftoversTable(const LeftoverStockEntry &v);
    void update_LeftoversTable();

    // results table
    //void addRow_ResultsTable(QString rodNumber, const CutPlan& plan);
    void update_ResultsTable(const QVector<CutPlan> &plans);
    void clear_ResultsTable();

private slots:
    void on_btnAddRow_clicked();
    void on_btnOptimize_clicked();
    void on_btnFinalize_clicked();
    void on_btnDisposal_clicked();
    void on_btnNewPlan_clicked();
    void on_btnClearPlan_clicked();
    void on_btnAddStockEntry_clicked();

private:
    Ui::MainWindow *ui;
    CuttingPresenter* presenter;
    CutAnalyticsPanel* analyticsPanel;

    // input tábla - vágási terv
    std::unique_ptr<InputTableManager> inputTableManager;
    // stock tábla - anyagkészlet - alapanyag raktárkészlet
    std::unique_ptr<StockTableManager> stockTableManager;
    // ♻️ Hullókezelő
    std::unique_ptr<LeftoverTableManager> leftoverTableManager;
    // eredmény
    std::unique_ptr<ResultsTableManager> resultsTableManager;

    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *e) override;

    void Connect_InputTableManager();
    void Connect_StockTableManager();
    void Connect_LeftoverTableManager();
};
#endif // MAINWINDOW_H
