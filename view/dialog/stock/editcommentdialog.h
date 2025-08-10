#pragma once

#include <QDialog>
#include <QString>

namespace Ui {
class EditCommentDialog;
}

class EditCommentDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditCommentDialog(QWidget* parent = nullptr);
    ~EditCommentDialog();

    void setModel(const QString& comment);
    QString comment() const;

private:
    Ui::EditCommentDialog* ui;
};
