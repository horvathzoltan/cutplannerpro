#include "addinputdialog.h"

#include "qpushbutton.h"
#include "ui_addinputdialog.h"
#include "model/materialregistry.h"

AddInputDialog::AddInputDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddInputDialog)
{
    ui->setupUi(this);
    populateMaterialCombo();

    // ui->comboCategory->addItem("RollerTube");
    // ui->comboCategory->addItem("BottomBar");

    // // Ok gomb disable ha nincs értelmes adat
    // connect(ui->editLength, &QLineEdit::textChanged, this, [=]() {
    //     ui->buttonBox->button(QDialogButtonBox::Ok)
    //         ->setEnabled(!ui->editLength->text().isEmpty());
    // });
}

AddInputDialog::~AddInputDialog()
{
    delete ui;
}


void AddInputDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().all();

    ui->comboMaterial->clear();
    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.displayName(), m.id);
    }
}

QUuid AddInputDialog::selectedMaterialId() const {
    return ui->comboMaterial->currentData().toUuid();
}

int AddInputDialog::length() const
{
    return ui->editLength->text().toInt();
}

int AddInputDialog::quantity() const
{
    return ui->spinQuantity->value();
}


