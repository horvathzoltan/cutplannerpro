#pragma once

#include <QDialog>

namespace Ui {
class ScrapByBarcodeDialog;
}

class ScrapByBarcodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScrapByBarcodeDialog(QWidget *parent = nullptr);
    ~ScrapByBarcodeDialog();

    QString barcode() const;
    bool repeat() const;

    void clearBarcodeField();

protected:
    void showEvent(QShowEvent *event) override;

private:
    Ui::ScrapByBarcodeDialog *ui;
};
