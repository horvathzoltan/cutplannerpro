#include "addwastedialog.h"
#include "ui_addwastedialog.h"
#include "model/registries/materialregistry.h"
#include <QMessageBox>

AddWasteDialog::AddWasteDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddWasteDialog)
    , current_entryId(QUuid::createUuid()) // 🔑 Automatikusan új UUID
{
    ui->setupUi(this);
    populateMaterialCombo();
}

AddWasteDialog::~AddWasteDialog()
{
    delete ui;
}

void AddWasteDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().readAll();
    ui->comboMaterial->clear();

    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.displayName(), m.id);
    }
}

QUuid AddWasteDialog::selectedMaterialId() const {
    return ui->comboMaterial->currentData().toUuid();
}

QString AddWasteDialog::barcode() const {
    return ui->editBarcode->text().trimmed();
}

int AddWasteDialog::availableLength() const {
    return ui->editLength->text().toInt(); // validáció később
}

QString AddWasteDialog::comment() const {
    return ui->editComment->text().trimmed();
}

Cutting::Result::LeftoverSource AddWasteDialog::source() const {
    return static_cast<Cutting::Result::LeftoverSource>(ui->editSourceValue->text().toInt()); // vagy szöveges → értelmezés
}

LeftoverStockEntry AddWasteDialog::getModel() const {
    LeftoverStockEntry entry;
    entry.entryId = current_entryId;
    entry.materialId = selectedMaterialId();
    entry.availableLength_mm = availableLength();
    entry.barcode = barcode();
    entry.source = source();
    entry.optimizationId = std::nullopt;
    return entry;
}

void AddWasteDialog::setModel(const LeftoverStockEntry& entry) {
    //currentBarcode = entry.barcode;
    current_entryId = entry.entryId; // ha szerkesztés módban vagyunk
    ui->editBarcode->setText(entry.barcode);
    ui->editLength->setText(QString::number(entry.availableLength_mm));
    ui->editSourceValue->setText(entry.sourceAsString());

    int index = ui->comboMaterial->findData(entry.materialId);
    if (index >= 0)
        ui->comboMaterial->setCurrentIndex(index);
}

bool AddWasteDialog::validateInputs() {
    if (selectedMaterialId().isNull()) {
        QMessageBox::warning(this, "Anyag hiányzik", "Válassz ki egy anyagot!");
        return false;
    }

    if (availableLength() <= 0) {
        QMessageBox::warning(this, "Hibás hossz", "Az elérhető hossz csak pozitív szám lehet!");
        return false;
    }

    if (barcode().isEmpty()) {
        QMessageBox::warning(this, "Hiányzó vonalkód", "Add meg a hulladékdarab azonosítóját!");
        return false;
    }

    return true;
}

void AddWasteDialog::accept() {
    if (!validateInputs())
        return;

    QDialog::accept();
}
