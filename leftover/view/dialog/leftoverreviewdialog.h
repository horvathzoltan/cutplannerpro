#ifndef LEFTOVERREVIEWDIALOG_H
#define LEFTOVERREVIEWDIALOG_H

#include <QDialog>

namespace Ui {
class LeftoverReviewDialog;
}

class LeftoverReviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LeftoverReviewDialog(QWidget *parent = nullptr);
    ~LeftoverReviewDialog();

    QString barcode() const;
    bool repeat() const;
    void clearBarcodeField();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::LeftoverReviewDialog *ui;
};

#endif // LEFTOVERREVIEWDIALOG_H
