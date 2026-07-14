#include "leftoverpresenter.h"
#include "common/logger.h"
#include "view/dialog/waste/leftoverreviewdialog.h"
#include "view/managers/leftovertable_manager.h"
#include <QMessageBox>
#include <model/registries/leftoverstockregistry.h>

LeftoverPresenter::LeftoverPresenter(LeftoverTableManager* mgr)
    : _mgr(mgr)
{}

void LeftoverPresenter::Review() {
    LeftoverReviewDialog dlg;

    while (dlg.exec() == QDialog::Accepted) {

        QString auditCode = dlg.barcode().trimmed();
        if (auditCode.isEmpty()) {
            if (!dlg.repeat()) break;
            dlg.clearBarcodeField();
            continue;
        }

        processAuditCode(auditCode);

        if (!dlg.repeat()) break;
        dlg.clearBarcodeField();
    }
}

void LeftoverPresenter::processAuditCode(const QString& auditCode)
{
    bool isPresent = auditCode.endsWith("+");
    bool isMissing = auditCode.endsWith("-");

    if (!isPresent && !isMissing) {
        QMessageBox::warning(nullptr, "Invalid code",
                             "Audit code must end with + or -");
        return;
    }

    QString original = auditCode.left(auditCode.length() - 1);

    auto entryOpt = LeftoverStockRegistry::instance().findByBarcode(original);
    if (!entryOpt) {
        QMessageBox::warning(nullptr, "Not found",
                             "No leftover found with this barcode.");
        return;
    }

    auto entry = *entryOpt;

    if (isPresent) {
        LeftoverStockRegistry::instance().markSeen(entry.entryId);
    } else {
        LeftoverStockRegistry::instance().markNotFound(entry.entryId);
    }

    // újra lekérjük a frissített entry-t
    auto updated = LeftoverStockRegistry::instance().findById(entry.entryId);
    if (updated) {
        _mgr->updateRow(*updated);
    }
}


void LeftoverPresenter::ReviewForm(){
    zTrace();
}