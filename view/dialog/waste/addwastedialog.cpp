#include "addwastedialog.h"
#include "ui_addwastedialog.h"
#include "materials/registry/material_registry.h"
#include <QMessageBox>
#include <model/registries/storageregistry.h>

AddWasteDialog::AddWasteDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddWasteDialog)
    , current_entryId(QUuid::createUuid()) // 🔑 Automatikusan új UUID
{
    ui->setupUi(this);
    populateMaterialCombo();
        populateStorageCombo();   // 🆕
}

AddWasteDialog::~AddWasteDialog()
{
    delete ui;
}

void AddWasteDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().readAll();
    ui->comboMaterial->clear();

    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.toDisplay(), m.id);
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

// QString AddWasteDialog::comment() const {
//     return ui->editComment->text().trimmed();
// }

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

    // 🆕 STORAGE
    QUuid sid = selectedStorageId();
    entry.storageId = sid.isNull() ? current_storageId : sid;

    zInfo(QString("USER CREATED LEFTOVER: entryId=%1, material=%2, length=%3, barcode=%4")
              .arg(entry.entryId.toString())
              .arg(entry.materialId.toString())
              .arg(entry.availableLength_mm)
              .arg(entry.barcode));

    return entry;
}

void AddWasteDialog::setModel(const LeftoverStockEntry& entry) {
    current_entryId = entry.entryId; // ha szerkesztés módban vagyunk
    current_storageId  = entry.storageId;

    ui->editBarcode->setText(entry.barcode);
    ui->editLength->setText(QString::number(entry.availableLength_mm));
    ui->editSourceValue->setText(entry.sourceAsString());

    int index = ui->comboMaterial->findData(entry.materialId);
    if (index >= 0)
        ui->comboMaterial->setCurrentIndex(index);

    if (!entry.storageId.isNull()) {
        int sidx = ui->comboStorage->findData(entry.storageId);
        if (sidx >= 0)
            ui->comboStorage->setCurrentIndex(sidx);
    }
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

        // ❗ Hullóknál a storage opcionális → NEM ellenőrizzük

    return true;
}

void AddWasteDialog::accept() {
    if (!validateInputs())
        return;

    QDialog::accept();
}

void AddWasteDialog::populateStorageCombo() {
    ui->comboStorage->clear();
    const auto& storages = StorageRegistry::instance().readAll();
    for (const auto& s : storages) {
        ui->comboStorage->addItem(s.name, s.id);
    }
}

QUuid AddWasteDialog::selectedStorageId() const {
    return ui->comboStorage->currentData().toUuid();
}
