#include <QLineEdit>
#include <QTableWidget>
#include "materialdelegate.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"
#include "materialfinderdialog.h"

#include <model/registries/stockregistry.h>

#include <QStandardItemModel>
#include <ui_MaterialFinderDialog.h>

#include <materials/registry/material_registry.h>

#include <common/settingsmanager.h>

MaterialFinderDialog::~MaterialFinderDialog()
{
    delete ui;
}

MaterialFinderDialog::MaterialFinderDialog(QWidget* parent)
    : QDialog(parent),
    ui(new Ui::MaterialFinderDialog)
{
    ui->setupUi(this);

    // 1️⃣ Delegate + model beállítása
    auto* model = new QStandardItemModel(this);
    ui->comboMaterial->setModel(model);                 // <-- EZ A HELYES
    ui->comboMaterial->setItemDelegate(new MaterialDelegate(this));

    // 2️⃣ Anyaglista feltöltése
    auto materials = MaterialRegistry::instance().readAll();
    for (const auto& m : materials) {
        QStandardItem* item = new QStandardItem();
        item->setData(QVariant::fromValue(m), Qt::UserRole);
        item->setData(m.name, Qt::DisplayRole); // fallback
        model->appendRow(item);
    }

    // 3️⃣ Spinbox
    ui->spinMinLength->setMinimum(0);
    ui->spinMinLength->setMaximum(100000);

    // 4️⃣ OK / Cancel
    connect(ui->btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    connect(ui->btn_MaterialSearch, &QPushButton::clicked, this, [this]() {
        MaterialSearchDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            auto sel = dlg.selection();

            // A kiválasztott anyag beállítása a comboBox-ban
            for (int i = 0; i < ui->comboMaterial->count(); ++i) {
                MaterialMaster m = ui->comboMaterial->itemData(i, Qt::UserRole).value<MaterialMaster>();
                if (m.id == sel.id) {
                    ui->comboMaterial->setCurrentIndex(i);
                    break;
                }
            }
        }
    });

    // Default range a Settingsből
    int defaultRange = SettingsManager::instance().materialFinderRange();

    // Debounce timerek
    auto* minDebounce = new QTimer(this);
    minDebounce->setInterval(300);
    minDebounce->setSingleShot(true);

    auto* maxDebounce = new QTimer(this);
    maxDebounce->setInterval(300);
    maxDebounce->setSingleShot(true);

    // MIN → MAX
    connect(ui->spinMinLength, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [=](int) {
                minDebounce->start();
            });

    connect(minDebounce, &QTimer::timeout, this, [=]() {
        int minVal = ui->spinMinLength->value();
        int maxVal = ui->spinBox->value();

        // Ha még nincs max → állítsuk be defaultRange alapján
        if (maxVal == 0) {
            ui->spinBox->setValue(minVal + defaultRange);
            return;
        }

        // Ha invalid → korrigáljuk
        if (minVal > maxVal)
            ui->spinBox->setValue(minVal);
    });

    // MAX → MIN
    connect(ui->spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [=](int) {
                maxDebounce->start();
            });

    connect(maxDebounce, &QTimer::timeout, this, [=]() {
        int minVal = ui->spinMinLength->value();
        int maxVal = ui->spinBox->value();

        // Ha még nincs min → állítsuk be defaultRange alapján
        if (minVal == 0) {
            int newMin = maxVal - defaultRange;
            if (newMin < 0) newMin = 0;
            ui->spinMinLength->setValue(newMin);
            return;
        }

        // Ha invalid → korrigáljuk
        if (maxVal < minVal)
            ui->spinMinLength->setValue(maxVal);
    });

}

MaterialFinderInput MaterialFinderDialog::getInput() const
{
    MaterialFinderInput r;

    MaterialMaster mat =
        ui->comboMaterial->currentData(Qt::UserRole).value<MaterialMaster>();

    r.materialId = mat.id;
    r.minLen = ui->spinMinLength->value();
    r.maxLen = ui->spinBox->value();

    return r;
}

