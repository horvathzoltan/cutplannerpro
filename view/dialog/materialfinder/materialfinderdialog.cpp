#include "materialfinderdialog.h"
#include "ui_MaterialFinderDialog.h"

#include <QStandardItemModel>
#include <materials/registry/material_registry.h>
#include "settings/settingsmanager.h"
#include "materialdelegate.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"

MaterialFinderDialog::~MaterialFinderDialog()
{
    delete _ui;
}

MaterialFinderDialog::MaterialFinderDialog(QWidget* parent)
    : QDialog(parent),
    _ui(new Ui::MaterialFinderDialog)
{
    _ui->setupUi(this);

    // 🔗 Lánc gomb vizuális stílus
    _ui->btn_Link->setStyleSheet(R"(
    QToolButton {
        font-size: 20px;
        color: #888;
        background: transparent;
        border: none;
        padding: 2px;
    }
    QToolButton:checked {
        color: #0078d4;
        font-weight: bold;
        background: transparent;
        border: none;
    }
)");



    // Anyaglista
    auto* model = new QStandardItemModel(this);
    _ui->comboMaterial->setModel(model);
    _ui->comboMaterial->setItemDelegate(new MaterialDelegate(this));

    for (const auto& m : MaterialRegistry::instance().readAll()) {
        QStandardItem* item = new QStandardItem();
        item->setData(QVariant::fromValue(m), Qt::UserRole);
        item->setData(m.name, Qt::DisplayRole);
        model->appendRow(item);
    }

    connect(_ui->btn_Link, &QToolButton::toggled, this, [this](bool on){
        if (on)
            _ui->btn_Link->setText("🔗");      // aktív
        else
            _ui->btn_Link->setText("⛓️‍💥");   // inaktív
    });

    // OK / Cancel
    connect(_ui->btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(_ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    // Anyag kereső gomb
    connect(_ui->btn_MaterialSearch, &QPushButton::clicked, this, [this]() {
        MaterialSearchDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            auto sel = dlg.selection();
            for (int i = 0; i < _ui->comboMaterial->count(); ++i) {
                MaterialMaster m = _ui->comboMaterial->itemData(i, Qt::UserRole).value<MaterialMaster>();
                if (m.id == sel.id) {
                    _ui->comboMaterial->setCurrentIndex(i);
                    break;
                }
            }
        }
    });

    // Default range
    int range = SettingsManager::instance().materialFinderRange();

    // MIN → MAX
    connect(_ui->spin_MinLength, &QSpinBox::editingFinished, this, [this, range]() {

        int minVal = _ui->spin_MinLength->value();
        int maxVal = _ui->spin_MaxLength->value();

        // 1) MAX nem mehet min alá
        _ui->spin_MaxLength->setMinimum(minVal);

        // 2) Ha a MAX jelenlegi értéke kisebb → korrigáljuk
        if (maxVal < minVal) {
            _ui->spin_MaxLength->setValue(minVal);
            maxVal = minVal;
        }

        // 3) Ha a lánc aktív → automatikus MAX számítás
        if (_ui->btn_Link->isChecked()) {
            int newMax = minVal + range;
            _ui->spin_MaxLength->setValue(newMax);
            _lastAutoMax = newMax;
        }
    });


    // MAX → MIN
    connect(_ui->spin_MaxLength, &QSpinBox::editingFinished, this, [this, range]() {

        int minVal = _ui->spin_MinLength->value();
        int maxVal = _ui->spin_MaxLength->value();

        // 1) MIN nem mehet max fölé
        _ui->spin_MinLength->setMaximum(maxVal);

        // 2) Ha a MIN jelenlegi értéke nagyobb → korrigáljuk
        if (minVal > maxVal) {
            _ui->spin_MinLength->setValue(maxVal);
            minVal = maxVal;
        }

        // 3) Ha a lánc aktív → automatikus MIN számítás
        if (_ui->btn_Link->isChecked()) {
            int newMin = maxVal - range;
            if (newMin < 0) newMin = 0;
            _ui->spin_MinLength->setValue(newMin);
            _lastAutoMin = newMin;
        }
    });

}

MaterialFinderInput MaterialFinderDialog::getInput() const
{
    MaterialFinderInput r;

    MaterialMaster mat =
        _ui->comboMaterial->currentData(Qt::UserRole).value<MaterialMaster>();

    r.materialId = mat.id;
    r.minLen = _ui->spin_MinLength->value();
    r.maxLen = _ui->spin_MaxLength->value();

    return r;
}
