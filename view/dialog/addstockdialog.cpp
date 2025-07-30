#include "addstockdialog.h"
#include "ui_addstockdialog.h"
#include "model/registries/materialregistry.h"

#include <QMessageBox>

AddStockDialog::AddStockDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddStockDialog)
{
    ui->setupUi(this);
    populateMaterialCombo();
}

AddStockDialog::~AddStockDialog()
{
    delete ui;
}

void AddStockDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().all();
    ui->comboMaterial->clear();

    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.displayName(), m.id);
    }
}

QUuid AddStockDialog::selectedMaterialId() const {
    return ui->comboMaterial->currentData().toUuid();
}

int AddStockDialog::quantity() const {
    return ui->spinQuantity->value();
}

QString AddStockDialog::comment() const {
    return ui->editComment->text().trimmed();
}

StockEntry AddStockDialog::getModel() const {
    StockEntry entry;
    entry.materialId = selectedMaterialId();
    entry.quantity = quantity();
    return entry;
}

void AddStockDialog::setModel(const StockEntry& entry) {
    currentMaterialId = entry.materialId;

    int index = ui->comboMaterial->findData(entry.materialId);
    if (index >= 0)
        ui->comboMaterial->setCurrentIndex(index);

    ui->spinQuantity->setValue(entry.quantity);
    ui->editComment->setText(""); // opcionálisan beállítható
}

bool AddStockDialog::validateInputs() {
    if (selectedMaterialId().isNull()) {
        QMessageBox::warning(this, "Hiányzó adat", "Kérlek válassz anyagot.");
        return false;
    }

    if (quantity() <= 0) {
        QMessageBox::warning(this, "Hibás mennyiség", "A mennyiségnek pozitívnak kell lennie.");
        return false;
    }

    return true;
}

void AddStockDialog::accept() {
    if (!validateInputs())
        return;

    QDialog::accept();
}
