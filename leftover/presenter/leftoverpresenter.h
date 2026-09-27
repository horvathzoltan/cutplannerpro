#pragma once
#include "leftover/services/bundlesplitengine.h"
#include <QString>
#include <QUuid>

#include <leftover/model/leftoverstockentry.h>

#include <presenter/CuttingPresenter.h>

class LeftoverTableManager;
class MainWindow; // Előre deklaráljuk, hogy ne kelljen most includolni

class LeftoverPresenter
{
public:
    LeftoverPresenter(MainWindow* view, LeftoverTableManager* mgr);

    void Review();      // Szemle dialog
    void ReviewForm();  // később

    //void ReviewFormPdf();
    //void ExportReviewFormPdf_old();
    void ExportReviewFormPdf();

    void ExportLeftoverIntakeForm_Pdf();
    void ExportLeftoverIntakeForm();

    void ExportStorageAuditPdf(
        const QUuid &storageId,
        const QVector<QUuid> &materialIds);

    void ExportOptimizationLeftoverAuditPdf(
        const QHash<QUuid, QVector<QUuid>>& perMachine);

    void applyBundleSplit(const BundleSplitResult &r);
    bool remove_LeftoverStockEntry(const QUuid &entryId);
    void add_LeftoverStockEntry(const LeftoverStockEntry& entry);
    void update_LeftoverStockEntry(const LeftoverStockEntry &updated);
    void ExportSubstitutionAuditPdf(const QVector<LeftoverStockEntry>& list);

    void ExportOptimizationLeftoverAudit(
        const QHash<QUuid, CuttingPresenter::OptimizationLeftoverAuditStats>& stats);

    void ExportOptimizationLeftoverSubstitutionAudit(
        const QHash<QUuid, QVector<QUuid>>& perMachine);


    void runIterativeLeftoverAuditRound();
    void ExportIterativeAuditPdf(const QVector<LeftoverStockEntry> &targets, const QHash<QUuid, QVector<LeftoverStockEntry> > &candidateSets);
private:
    MainWindow* _view;


    void processAuditCode(const QString& auditCode);

    LeftoverTableManager* _mgr;

    void exportAuditPdf(const QVector<LeftoverStockEntry> &list, const QString &title);
    QVector<LeftoverStockEntry> shuffle(QVector<LeftoverStockEntry>& list, int rowsPerPage);
    QVector<LeftoverStockEntry> filter(const QVector<LeftoverStockEntry>& list);
    QString buildSubstitutionSummary(const QVector<LeftoverStockEntry> &targets, const QHash<QUuid, QVector<LeftoverStockEntry> > &candidateSets);
};
