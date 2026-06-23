#pragma once

#include <QDialog>
#include <QUuid>
#include "../../../model/leftoverstockentry.h"

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
    //QString comment() const;
    Cutting::Result::LeftoverSource source() const;

    void accept() override;
    LeftoverStockEntry getModel() const;
    void setModel(const LeftoverStockEntry& entry);
    bool shouldRepeat();

private:
    Ui::AddWasteDialog *ui;
    void populateMaterialCombo();
    bool validateInputs();

    //QString currentBarcode;
    QUuid current_entryId; // Az aktuális anyag ID, ha szerkesztés módban vagyunk

    QUuid current_storageId;     // 🆕
    void populateStorageCombo(); // 🆕
    QUuid selectedStorageId() const; // 🆕

    static QUuid s_lastMaterialId;
    static int   s_lastLength;
    static QUuid s_lastStorageId;
    static QString s_lastBarcode;   // 🆕

    static bool s_lastRepeat;

    int shadowManualCounter = 0; // shadow counter a manuális leftover ID-hez
    bool validateBarcodeFormat(const QString &bc) const;
};
