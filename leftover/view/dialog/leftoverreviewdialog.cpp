#include "leftoverreviewdialog.h"
#include "ui_leftoverreviewdialog.h"

#include <settings/settingsmanager.h>

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

    // Repeat checkbox – load saved state
    ui->chkRepeat->setChecked(
        SettingsManager::instance().repeatDialog_LeftoverReview()
        );

    // Persist repeat checkbox changes
    connect(ui->chkRepeat, &QCheckBox::toggled, this, [](bool checked){
        SettingsManager::instance().setRepeatDialog_LeftoverReview(checked);
    });

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
