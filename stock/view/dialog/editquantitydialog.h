#pragma once

#include <QDialog>

namespace Ui {
class EditQuantityDialog;
}

class EditQuantityDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditQuantityDialog(QWidget* parent = nullptr);
    ~EditQuantityDialog();

    void setData(int currentQuantity);
    int getData() const {return _quantity;}

    void accept() override;


private:
    Ui::EditQuantityDialog* ui;
    int _originalQuantity;
    int _quantity;

    int parsedQuantity(int q0) const;
};
