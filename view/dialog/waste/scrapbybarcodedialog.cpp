#include "scrapbybarcodedialog.h"
#include "ui_scrapbybarcodedialog.h"

ScrapByBarcodeDialog::ScrapByBarcodeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ScrapByBarcodeDialog)
{
    ui->setupUi(this);

    // Enter = OK
    ui->txtBarcode->setFocus();
    ui->txtBarcode->setPlaceholderText("Scan or type barcode...");

    // Optional: disable resizing
    setFixedSize(sizeHint());
}

ScrapByBarcodeDialog::~ScrapByBarcodeDialog()
{
    delete ui;
}

QString ScrapByBarcodeDialog::barcode() const
{
    return ui->txtBarcode->text().trimmed();
}

bool ScrapByBarcodeDialog::repeat() const
{
    return ui->chkRepeat->isChecked();
}

void ScrapByBarcodeDialog::clearBarcodeField()
{
    ui->txtBarcode->clear();
    ui->txtBarcode->setFocus();
}

void ScrapByBarcodeDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    ui->txtBarcode->setFocus();
}
