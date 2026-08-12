#pragma once
#include <QString>
#include <QUuid>

#include <leftover/model/leftoverstockentry.h>

class LeftoverTableManager;

class LeftoverPresenter
{
public:
    LeftoverPresenter(LeftoverTableManager* mgr);

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

private:
    void processAuditCode(const QString& auditCode);

    LeftoverTableManager* _mgr;

    void exportAuditPdf(const QVector<LeftoverStockEntry> &list, const QString &title);
};
