#pragma once

#include <QDialog>
#include <QUuid>
#include "model/stockentry.h"

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
    bool validateInputs();

    QUuid currentEntryId;
    int currentQuantity;
    int parsedQuantityDelta(int& resultValue) const;
};
