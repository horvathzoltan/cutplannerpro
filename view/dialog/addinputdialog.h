#pragma once

#include "model/cutting/plan/request.h"
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
    Cutting::Plan::Request getModel() const;

    void setModel(const Cutting::Plan::Request& request); // ⬅️ új metódus

private:
    Ui::AddInputDialog *ui;
    void populateMaterialCombo(); // új: feltölti a comboBox-ot törzsből
    bool validateInputs();

    QUuid current_requestId; // 🔒 Megőrzi az eredeti ID-t

};

