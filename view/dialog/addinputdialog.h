#pragma once

#include "model/cuttingplanrequest.h"
#include <QDialog>
#include <QUuid>

namespace Ui {
class AddInputDialog;
}

class AddInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddInputDialog(QWidget *parent = nullptr);
    ~AddInputDialog();

    QUuid selectedMaterialId() const;
    int length() const;
    int quantity() const;

    QString ownerName() const;
    QString externalReference() const;

    ;

    // ❗️Ide jön az override metódus
    void accept() override;
    CuttingPlanRequest getModel() const;

    void setModel(const CuttingPlanRequest& request); // ⬅️ új metódus

private:
    Ui::AddInputDialog *ui;
    void populateMaterialCombo(); // új: feltölti a comboBox-ot törzsből
    bool validateInputs();

    QUuid currentRequestId; // 🔒 Megőrzi az eredeti ID-t

};

