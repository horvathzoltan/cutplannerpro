#pragma once

#include <QDialog>
#include <QUuid>
#include <QVector>
#include "ui_leftoverauditdialog.h"

namespace Ui {
class LeftoverAuditDialog;
}

class LeftoverAuditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LeftoverAuditDialog(QWidget* parent = nullptr);
    ~LeftoverAuditDialog();

    QUuid selectedStorage() const;
    QVector<QUuid> selectedMaterials() const;

private:
    void loadStorages();
    void loadMaterials();

private:
    Ui::LeftoverAuditDialog* ui;
};
