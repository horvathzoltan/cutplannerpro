#pragma once

#include "model/cuttingrequest.h"
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
    CuttingRequest getModel() const;

private:
    Ui::AddInputDialog *ui;
    void populateMaterialCombo(); // új: feltölti a comboBox-ot törzsből
    bool validateInputs();
};

