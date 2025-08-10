#include "editquantitydialog.h"
#include "ui_editquantitydialog.h"
#include "common/quantityparser.h"

#include <QMessageBox>

EditQuantityDialog::EditQuantityDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::EditQuantityDialog)
{
    ui->setupUi(this);
}

EditQuantityDialog::~EditQuantityDialog() {
    delete ui;
}

void EditQuantityDialog::setData(int currentQuantity) {
    _originalQuantity = currentQuantity;
    ui->lineEditQuantity->setText(QString::number(_originalQuantity));
}

int EditQuantityDialog::parsedQuantity(int q0) const {
    int value = 0;
    auto mode = QuantityParser::parse(ui->lineEditQuantity->text().trimmed(), value);
    if (mode == QuantityParser::Relative)
        return q0 + value;
    if (mode == QuantityParser::Absolute)
        return value;
    return -1;
}

void EditQuantityDialog::accept()
{
    QString text = ui->lineEditQuantity->text().trimmed();

    _quantity = parsedQuantity(_originalQuantity);
    if (_quantity < 0) {
        QMessageBox::warning(this, "Hibás érték", "Kérlek, érvényes mennyiséget adj meg!");
        return;
    }
    // ✅ Bezárjuk a dialógust
    QDialog::accept();
}

