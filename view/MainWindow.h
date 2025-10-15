#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "model/cutting/result/resultmodel.h"
#include "managers/inputtable_manager.h"
#include "managers/leftovertable_manager.h"
#include "managers/resultstable_manager.h"
#include "managers/stocktable_manager.h"
#include "cutanalyticspanel.h"
#include "model/storageaudit/storageauditentry.h"
#include "presenter/CuttingPresenter.h"
#include "view/managers/cuttinginstructiontable_manager.h"
#include "view/managers/relocationplantable_manager.h"
#include "view/managers/storageaudittable_manager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

//class CuttingPresenter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateStats(const QVector<Cutting::Plan::CutPlan> &plans, const QVector<Cutting::Result::ResultModel> &results);
    void setInputFileLabel(const QString &label, const QString &tooltip);
    void ShowWarningDialog(const QString &msg);

    // input table
    void addRow_InputTable(const Cutting::Plan::Request &v);
    void updateRow_InputTable(const Cutting::Plan::Request &v);
    void removeRow_InputTable(const QUuid &v);
    void clear_InputTable();
    // stock table
    void addRow_StockTable(const StockEntry &v);
    void removeRow_StockTable(const QUuid &id);
    void updateRow_StockTable(const StockEntry &v);
    void refresh_StockTable();
    // leftovers table
    void removeRow_LeftoversTable(const QUuid &id);
    void addRow_LeftoversTable(const LeftoverStockEntry &v);
    void updateRow_LeftoversTable(const LeftoverStockEntry &v);
    void refresh_LeftoversTable();

    // results table
    //void addRow_ResultsTable(QString rodNumber, const CutPlan& plan);
    void update_ResultsTable(const QVector<Cutting::Plan::CutPlan> &plans);
    void clear_ResultsTable();

    void update_StorageAuditTable(const QVector<StorageAuditRow> &entries);
    void updateRow_StorageAuditTable(const StorageAuditRow &row);
    void showAuditCheckbox(const QUuid &rowId);
private slots:
    void handle_btn_NewCuttingPlan_clicked();
    void handle_btn_AddCuttingPlanRequest_clicked();
    void handle_btn_ClearCuttingPlan_clicked();
    void handle_btn_OpenCuttingPlan_clicked();

    void handle_btn_Optimize_clicked();
    void handle_btn_Finalize_clicked();
    void handle_btn_AddStockEntry_clicked();

    void handle_btn_LeftoverDisposal_clicked();
    void handle_btn_AddLeftoverStockEntry_clicked();

    void handle_btn_RelocationPlanFinalize_clicked();

    void on_btn_StorageAudit_clicked();

    void on_btn_Relocate_clicked();

    void on_btn_GenerateCuttingPlan_clicked();

private:
    Ui::MainWindow *ui;
    CuttingPresenter* presenter = nullptr;
    CutAnalyticsPanel* analyticsPanel = nullptr;

    // input tábla - vágási terv
    std::unique_ptr<InputTableManager> inputTableManager = nullptr;
    // stock tábla - anyagkészlet - alapanyag raktárkészlet
    std::unique_ptr<StockTableManager> stockTableManager = nullptr;
    // ♻️ Hullókezelő
    std::unique_ptr<LeftoverTableManager> leftoverTableManager = nullptr;
    // eredmény
    std::unique_ptr<ResultsTableManager> resultsTableManager = nullptr;
    // storage_audit
    std::unique_ptr<StorageAuditTableManager> storageAuditTableManager = nullptr;

    std::unique_ptr<RelocationPlanTableManager> relocationPlanTableManager = nullptr;

    std::unique_ptr<CuttingInstructionTableManager> cuttingInstructionTableManager = nullptr;


    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *e) override;
    void ButtonConnector_Connect();
    QString format(const QList<RelocationInstruction> &items);
    void initEventLogWidget();
};
#endif // MAINWINDOW_H
