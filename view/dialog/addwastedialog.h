#pragma once

#include <QDialog>
#include <QUuid>
#include "model/leftoverstockentry.h"

namespace Ui {
class AddWasteDialog;
}

class AddWasteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddWasteDialog(QWidget *parent = nullptr);
    ~AddWasteDialog();

    QUuid selectedMaterialId() const;
    QString barcode() const;
    int availableLength() const;
    QString comment() const;
    LeftoverSource source() const;

    void accept() override;
    LeftoverStockEntry getModel() const;
    void setModel(const LeftoverStockEntry& entry);

private:
    Ui::AddWasteDialog *ui;
    void populateMaterialCombo();
    bool validateInputs();

    //QString currentBarcode;
    QUuid current_entryId; // Az aktuális anyag ID, ha szerkesztés módban vagyunk
};
