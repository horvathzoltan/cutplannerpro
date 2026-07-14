#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "managers/inputtable_manager.h"
#include "managers/leftovertable_manager.h"
#include "managers/resultstable_manager.h"
#include "managers/stocktable_manager.h"
#include "../model/storageaudit/storageauditentry.h"
#include "../presenter/CuttingPresenter.h"
#include "managers/cuttinginstructiontable_manager.h"
#include "managers/relocationplantable_manager.h"
#include "managers/storageaudittable_manager.h"
#include "presenter/stockpresenter.h"
#include "presenter/leftoverpresenter.h"
#include "tableutils/highlightdelegate.h"
#include "view/dialog/input/series_matrix_view.h"

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

    //void updateStats(const QVector<Cutting::Plan::CutPlan> &plans, const QVector<Cutting::Result::ResultModel> &results);
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

    void renderCuttingInstructions(const QVector<MachineCuts>& list);

    void switchToCuttingPlanTab();
    void switchToInstructionsPlanTab();

    void refresh_InputTableFromRegistry();

    bool isChkUseLeftoversChecked();

    SeriesMatrixView* seriesMatrixView() const { return _seriesMatrixView; }
    StockPresenter* getStockPresenter() const { return stockPresenter; }
    StockTableManager* getStockTableManager() const { return stockTableManager.get(); }

private slots:
    void handle_btn_NewRequest_clicked();
    void handle_btn_AddCuttingPlanRequest_clicked();
    void handle_btn_ClearRequest_clicked();
    void handle_btn_OpenRequest_clicked();
    void handle_btn_CloneRequest_clicked();

    void handle_btn_Optimize_clicked();
    void handle_btn_ExportCutPlanSummary_clicked();

    void handle_btn_AddStockEntry_clicked();

    void handle_btn_LeftoverDisposal_clicked();
    void handle_btn_AddLeftoverStockEntry_clicked();
    void handle_btn_ExportLeftoverForm_clicked();

    void handle_btn_RelocationPlanFinalize_clicked();

    void handle_btn_StorageAudit_clicked();

    void handle_btn_Relocate_clicked();

    void handle_btn_GenerateCuttingPlan_clicked();

    void handle_btn_OptRad_clicked(bool checked);

    void handle_btn_ExportCutInstruction_clicked();

    void handle_btn_Painter_clicked();
    void handle_btn_Audit_clicked();
    void handle_btn_BOMaudit_clicked();

    void onRowFinalized(int rowIx);
    void onCompensationChanged(const QUuid& machineId, double newVal);

    void handle_act_MaterialFinder_clicked();
    void handle_act_Settings_clicked();

    void onHighlightLeftover(const QUuid& id);
    void onHighlightStock(const QUuid& id);
    void onShowNotFoundMessage(const QString& msg);

    void onSeriesMatrixClosed();

    void handle_btn_ReviewForm_clicked();
    void handle_btn_Review_clicked();

private:
    Ui::MainWindow *ui;
    CuttingPresenter* presenter = nullptr;
    StockPresenter* stockPresenter = nullptr;
    LeftoverPresenter* leftoverPresenter = nullptr;

    //CutAnalyticsPanel* analyticsPanel = nullptr;
    HighlightDelegate *_highlightDelegate;
    SeriesMatrixView* _seriesMatrixView = nullptr;
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


    QAction* _actSeriesMatrix = nullptr;

private:
    static constexpr int TAB_CUTREQUEST        = 0;
    static constexpr int TAB_CUTTINGPLAN       = 1;
    static constexpr int TAB_STORAGEAUDIT      = 2;
    static constexpr int TAB_RELOCATION        = 3;
    static constexpr int TAB_CUTINSTRUCTION    = 4;
    static constexpr int TAB_STOCK             = 5;
    static constexpr int TAB_LEFTOVER          = 6;


    void closeEvent(QCloseEvent *event) override;
    bool event(QEvent *e) override;
    void ButtonConnector_Connect();
    QString format(const QList<RelocationInstruction> &items);
    void initEventLogWidget();
    void translate();
    void refreshSummaryRows();
    void postProcessMachineCuts(MachineCuts &mc);
    //void renderCuttingInstructions();
    //static QStringList generateStatsStrings(const QVector<Cutting::Plan::CutPlan> &plans, const QVector<Cutting::Result::ResultModel> &leftovers);

    struct ActionConnectorModel{
        QAction* actMaterialFinder = nullptr;
        QAction* actSettings = nullptr;
        QAction* actSeriesMatrix = nullptr;
    };

    void ActionConnector_connect(ActionConnectorModel& a);
    void mainToolbarBuilder(ActionConnectorModel& m1);
    void buildStorageTree();
};
#endif // MAINWINDOW_H
