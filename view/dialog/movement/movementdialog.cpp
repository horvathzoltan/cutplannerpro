#include "movementdialog.h"
#include "ui_movementdialog.h"
#include "model/registries/storageregistry.h"

MovementDialog::MovementDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::MovementDialog)
{
    ui->setupUi(this);

    // Töltés: cél tárolók
    const auto& storages = StorageRegistry::instance().readAll();
    for (const auto& s : storages) {
        ui->comboTargetStorage->addItem(s.name, QVariant::fromValue(s.id));
    }

    // Mennyiség validálás
    ui->spinQuantity->setMinimum(1);

    // Gombok
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

MovementDialog::~MovementDialog() {
    delete ui;
}

void MovementDialog::setSource(const QString& storageName, const QUuid& entryId, int availableQuantity) {
    sourceEntryId = entryId;
    maxQuantity = availableQuantity;

    ui->lblSourceStorageValue->setText(storageName);
    ui->spinQuantity->setMaximum(availableQuantity);
}

MovementData MovementDialog::getMovementData() const {
    MovementData data;
    data.fromEntryId = sourceEntryId;
    data.toStorageId = ui->comboTargetStorage->currentData().toUuid();
    data.quantity = ui->spinQuantity->value();
    data.comment = ui->editComment->toPlainText();
    return data;
}
