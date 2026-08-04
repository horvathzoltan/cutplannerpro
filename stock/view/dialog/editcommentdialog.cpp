#include "editcommentdialog.h"
#include "ui_editcommentdialog.h"

#include <QMessageBox>

EditCommentDialog::EditCommentDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::EditCommentDialog)
{
    ui->setupUi(this);
}

EditCommentDialog::~EditCommentDialog() {
    delete ui;
}

void EditCommentDialog::setModel(const QString& comment) {
    _originalComment = comment;
    ui->editComment->setPlainText(_originalComment);
}

QString EditCommentDialog::comment() const {
    return _comment;
}

void EditCommentDialog::accept()
{
    QString text = ui->editComment->toPlainText().trimmed();

    // 🛑 Validáció: ne legyen üres
    if (text.isEmpty()) {
        QMessageBox::warning(this, "Üres megjegyzés", "Kérlek, adj meg egy érvényes megjegyzést!");
        return;
    }

    // 🛑 Validáció: max 200 karakter
    if (text.length() > 200) {
        QMessageBox::warning(this, "Túl hosszú megjegyzés", "A megjegyzés legfeljebb 200 karakter lehet.");
        return;
    }

    _comment = text;
    QDialog::accept();
}
