#pragma once

#include <QDialog>
#include <QUuid>

namespace Ui {
class EditStorageDialog;
}

class EditStorageDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditStorageDialog(QWidget* parent = nullptr);
    ~EditStorageDialog();

    QUuid selectedStorageId() const;
    void setInitialStorageId(const QUuid& id);  // volt: setModel(...)

    void accept() override;  // ha van validáció, akkor érdemes lehet override-olni

private:
    Ui::EditStorageDialog* ui;
    void populateStorageCombo();
    bool validateInputs();  // javasolt új metódus

    QUuid initialStorageId_;
    QUuid selectedStorageId_;  // ha van külön tárolt kiválasztott elem
};
