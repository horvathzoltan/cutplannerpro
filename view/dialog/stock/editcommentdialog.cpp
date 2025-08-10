#include "editcommentdialog.h"
#include "ui_editcommentdialog.h"

EditCommentDialog::EditCommentDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::EditCommentDialog)
{
    ui->setupUi(this);
}

EditCommentDialog::~EditCommentDialog() {
    delete ui;
}

void EditCommentDialog::setModel(const QString& comment) {
    ui->editComment->setText(comment);
}

QString EditCommentDialog::comment() const {
    return ui->editComment->toPlainText().trimmed();
}
