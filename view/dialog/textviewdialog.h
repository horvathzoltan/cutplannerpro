#ifndef TEXTVIEWDIALOG_H
#define TEXTVIEWDIALOG_H

#include <QDialog>

namespace Ui {
class TextViewDialog;
}

class TextViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TextViewDialog(QWidget *parent = nullptr);
    ~TextViewDialog();

    void setText(const QString &txt);
private:
    Ui::TextViewDialog *ui;
};

#endif // TEXTVIEWDIALOG_H
