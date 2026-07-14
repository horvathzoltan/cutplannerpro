#pragma once
#include <QString>

class LeftoverTableManager;

class LeftoverPresenter
{
public:
    LeftoverPresenter(LeftoverTableManager* mgr);

    void Review();      // Szemle dialog
    void ReviewForm();  // később

private:
    void processAuditCode(const QString& auditCode);

    LeftoverTableManager* _mgr;
};
