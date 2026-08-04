#pragma once

#include "stock/model/stockentry.h"
#include <QDialog>
#include <QUuid>

namespace Ui {
class AddStockDialog;
}

class AddStockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddStockDialog(QWidget *parent = nullptr);
    ~AddStockDialog();

    QUuid selectedMaterialId() const;
    int quantity() const;
    QString comment() const;

    void accept() override;
    StockEntry getModel() const;
    void setModel(const StockEntry& entry);

private:
    Ui::AddStockDialog *ui;
    void populateMaterialCombo();
    void populateStorageCombo();

    bool validateInputs();

    QUuid current_entryId;
    QUuid current_storageId; // Új mező a tárolóhoz
    int currentQuantity;
    //int parsedQuantityDelta(int& resultValue) const;
    QUuid selectedStorageId() const;

    static QUuid s_lastMaterialId;
    static QUuid s_lastStorageId;
    static int   s_lastQuantity;
    static QString s_lastComment;

};
