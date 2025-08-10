#include "editstoragedialog.h"
#include "ui_editstoragedialog.h"

#include <QMessageBox>

#include <model/registries/storageregistry.h>

EditStorageDialog::EditStorageDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::EditStorageDialog)
{
    ui->setupUi(this);
    populateStorageCombo();

    // Beállítjuk az initialStorageId-t a ComboBoxban, ha elérhető
    // Ez a logika átkerülhet később a setInitialStorageId metódusba is
}

EditStorageDialog::~EditStorageDialog()
{
    delete ui;
}

void EditStorageDialog::setInitialStorageId(const QUuid& id)
{
    initialStorageId_ = id;

    // Kiválasztjuk a ComboBoxban az adott storage ID-t
    for (int i = 0; i < ui->comboStorage->count(); ++i) {
        QVariant data = ui->comboStorage->itemData(i);
        if (data.isValid() && data.toUuid() == initialStorageId_) {
            ui->comboStorage->setCurrentIndex(i);
            break;
        }
    }
}

QUuid EditStorageDialog::selectedStorageId() const
{
    QVariant data = ui->comboStorage->currentData();
    return data.isValid() ? data.toUuid() : QUuid{};
}

void EditStorageDialog::populateStorageCombo() {
    ui->comboStorage->clear();
    const auto& storages = StorageRegistry::instance().readAll();
    for (const auto& s : storages) {
        ui->comboStorage->addItem(s.name, s.id);
    }
}

bool EditStorageDialog::validateInputs()
{
    if (!selectedStorageId().isNull()) {
        return true;
    }

    QMessageBox::warning(this, "Hiba", "Válassz ki egy tárolót.");
    return false;
}

void EditStorageDialog::accept()
{
    if (!validateInputs()) {
        return;  // Ne zárjuk be a dialógust hibás input esetén
    }

    QDialog::accept();
}
