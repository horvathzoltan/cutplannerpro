#pragma once

#include <QObject>
#include <model/cutting/instruction/cutinstruction.h>
#include <view/dialog/input/clonerequestdialog.h>
#include "../model/cutting/optimizer/optimizermodel.h"
#include "../model/archivedwasteentry.h"
#include "../model/relocation/relocationinstruction.h"
#include "cutting/export/sortmode.h"

/*
Értelmezi és kezeli a felhasználói interakciókat

Meghívja a modell metódusait (pl. optimize())

Átadja a View-nak a megjelenítendő adatokat (pl. vágási terv, maradékok)
*/
class MainWindow; // Előre deklaráljuk, hogy ne kelljen most includolni

class CuttingPresenter : public QObject {
    Q_OBJECT

public:
    explicit CuttingPresenter(MainWindow* view, QObject *parent = nullptr);

    struct CuttingRequestPatch {        
        bool updateLength        = false;
        bool updateMaterial      = false;
        bool updateLeftRight     = false;

        bool updateWidthHeight = false;
        bool updateOwner         = false;
        bool updateDueDate       = false;
        bool updateProductType   = false;
        bool updateProductSubtype= false;
        bool updateColor         = false;
        bool updateAttributes    = false;
    };

    // Vágási igények
    void add_CuttingPlanRequest(const Cutting::Plan::Request& req);
    void update_CuttingPlanRequest(const Cutting::Plan::Request& updated);
    void remove_CuttingPlanRequest(const QUuid &id);

    void cloneRequestDialog();
    void cloneRequest(const QVector<CloneMaterialRule>& rules, const QString& tag);

    //
    void removeAll_CuttingPlanRequests();

    void createNew_CuttingPlanRequests();

    void setCuttingRequests(const QVector<Cutting::Plan::Request> &list);

    // Készlet
    void setStockInventory(const QVector<StockEntry> &list);

    void add_StockEntry(const StockEntry &entry);
    void remove_StockEntry(const QUuid &stockId);
    void update_StockEntry(const StockEntry &updated);

    // Úrafelhasználható - hulló anyagok készlete
    void setReusableInventory(const QVector<LeftoverStockEntry> &list);

    // Paraméterek
    //void setKerf(int kerf);

    // Optimalizálás
    void runOptimization(Cutting::Optimizer::TargetHeuristic h);

    // Eredmények lekérése
    const QVector<Cutting::Plan::CutPlan>& getPlansRef() const;
    QVector<Cutting::Result::ResultModel> getLeftoverResults();
    void finalizePlans();
    void scrapShortLeftovers();
    void exportArchivedWasteToCSV(const QVector<ArchivedWasteEntry> &entries);

    void syncModelWithRegistries();
    bool loadCuttingPlanFromFile(const QString &path);

    QVector<RelocationInstruction> generateRelocationPlan(
        const QVector<Cutting::Plan::CutPlan>& cutPlans,
        const QVector<StorageAuditRow>& auditRows);

    const QVector<MachineCuts> machineCutsList() const {return _machineCutsList;}
    //const QVector<StorageAuditRow>& getLastAuditRows() const { return lastAuditRows;}


    //AuditStateManager* auditStateManager() { return &_auditStateManager;}


    //void ExportCutPlanSummary();
    void GenerateCutInstructions(SortMode mode,
                                 const QVector<QString>& prioRefs);
    // void ExportCutInstructions();
    // void ExportCutInstructions_2();
    // void ExportCutInstructions_Labels(const QString& path, QMap<QUuid, QVector<const CutInstruction*>> orderedCuts2);

    //QVector<QString> resolveTargetStorages(const QUuid &rootStorageId);
    void UpdateCompensation(const QUuid &machineId, double newVal);

    // static constexpr int printedLineWidth = 75;
    // static constexpr int printedPageHeight = 60;


    //QStringList BOM_audit();
    void update_AllRequestsWithSameReference(const Cutting::Plan::Request &updated);
    const Cutting::Optimizer::OptimizerModel* optimizerModel() const { return &_optimizerModel; }

    void loadLatestSnapshotForCurrentPlan();
    void saveOptimizationSnapshot();

    class Refresh {
    public:
        enum class Flags : uint32_t {
            None            = 0,
            InputTable      = 1 << 0,
            StockTable      = 1 << 1,
            LeftoversTable  = 1 << 2,
            ResultsTable    = 1 << 3,
            SwitchToTab     = 1 << 4,

            RequestOnly     = InputTable | StockTable | LeftoversTable,
            SnapshotOnly    = ResultsTable | SwitchToTab,
            All             = RequestOnly | SnapshotOnly
        };

        // ⭐ Ezek NEM tagfüggvények, hanem friend-ek → nem kapnak implicit this-t
        friend inline Flags operator|(Flags a, Flags b) {
            return static_cast<Flags>(
                static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
                );
        }

        friend inline bool hasFlag(Flags flags, Flags flag) {
            return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
        }
    };


    void refreshAllViews(Refresh::Flags flags);
    QHash<QUuid, QVector<QUuid>> collectUsedLeftoversFromPlans();
    bool isOptimizationAuditRequired(const QVector<QUuid> &usedIds);

    struct OptimizationLeftoverAuditStats {
        int missing = 0;   // notFoundCount > 0
        int stale = 0;     // lastSeenAt túl régi
        int fresh = 0;     // minden rendben
    };

    QHash<QUuid, OptimizationLeftoverAuditStats> collectOptimizationLeftoverStats(
        const QHash<QUuid, QVector<QUuid> > &perMachine);

private:
    void applyPatch(Cutting::Plan::Request& target,
                    const Cutting::Plan::Request& updated,
                    const CuttingRequestPatch& patch);

private:
    MainWindow* _view;
    Cutting::Optimizer::OptimizerModel _optimizerModel;
    QVector<MachineCuts> _machineCutsList;

    bool isModelSynced = false;
    QMap<QUuid, int> generatePickingMapFromPlans(const QVector<Cutting::Plan::CutPlan> &plans);
    //void logPlans();
    //AuditStateManager _auditStateManager;

    //static RelocationInstruction makeRelocationInstruction(const QString &materialName, const QUuid &materialId, const QString &barcode, int plannedQuantity, AuditSourceType sourceType, const StorageAuditRow &sourceRow, const QUuid &targetRootId, const QString &targetName, int moveQty);
    void updateConfirmedCount(StorageAuditRow &row, bool wasModifiedBefore);
};



