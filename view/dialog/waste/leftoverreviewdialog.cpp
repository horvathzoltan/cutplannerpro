#include "leftoverreviewdialog.h"
#include "ui_leftoverreviewdialog.h"

// LeftoverReviewDialog::LeftoverReviewDialog(QWidget *parent)
//     : QDialog(parent)
//     , ui(new Ui::LeftoverReviewDialog)
// {
//     ui->setupUi(this);
// }

// LeftoverReviewDialog::~LeftoverReviewDialog()
// {
//     delete ui;
// }



LeftoverReviewDialog::LeftoverReviewDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LeftoverReviewDialog)
{
    ui->setupUi(this);

    // Enter = OK
    ui->txtBarcode->setFocus();
    ui->txtBarcode->setPlaceholderText("Scan or type barcode...");

    // Optional: disable resizing
    setFixedSize(sizeHint());
}

LeftoverReviewDialog::~LeftoverReviewDialog()
{
    delete ui;
}

QString LeftoverReviewDialog::barcode() const
{
    return ui->txtBarcode->text().trimmed();
}

bool LeftoverReviewDialog::repeat() const
{
    return ui->chkRepeat->isChecked();
}

void LeftoverReviewDialog::clearBarcodeField()
{
    ui->txtBarcode->clear();
    ui->txtBarcode->setFocus();
}

void LeftoverReviewDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    ui->txtBarcode->setFocus();
}
