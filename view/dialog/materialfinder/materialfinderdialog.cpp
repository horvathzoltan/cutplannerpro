#include <QLineEdit>
#include <QTableWidget>
#include "materialdelegate.h"
#include "materialfinderdialog.h"

#include <model/registries/stockregistry.h>

#include <QStandardItemModel>
#include <ui_MaterialFinderDialog.h>

#include <materials/registry/material_registry.h>

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
}

MaterialFinderInput MaterialFinderDialog::getInput() const
{
    MaterialFinderInput r;

    MaterialMaster mat =
        ui->comboMaterial->currentData(Qt::UserRole).value<MaterialMaster>();

    r.materialId = mat.id;
    r.minLength = ui->spinMinLength->value();

    return r;
}

