#pragma once
#include <QString>

class LeftoverTableManager;

class LeftoverPresenter
{
public:
    LeftoverPresenter(LeftoverTableManager* mgr);

    void Review();      // Szemle dialog
    void ReviewForm();  // később

    //void ReviewFormPdf();
    void ExportReviewFormPdf();   // ⬅ ÚJ

private:
    void processAuditCode(const QString& auditCode);

    LeftoverTableManager* _mgr;
};
