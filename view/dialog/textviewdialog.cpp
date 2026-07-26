#include "textviewdialog.h"
#include "ui_textviewdialog.h"

TextViewDialog::TextViewDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TextViewDialog)
{
    ui->setupUi(this);
}

TextViewDialog::~TextViewDialog()
{
    delete ui;
}


void TextViewDialog::setText(const QString& txt)
{
    ui->plainTextEdit->setPlainText(txt);
}
