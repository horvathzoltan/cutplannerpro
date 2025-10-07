#include "relocationquantitydialog.h"
#include "ui_relocationquantitydialog.h"

#include <QPushButton>
#include <QSpinBox>

RelocationQuantityDialog::RelocationQuantityDialog(QWidget* parent)
    : QDialog(parent), _ui(new Ui::RelocationQuantityDialog) {
    _ui->setupUi(this);

    //connect(ui->btnDistributeEvenly, &QPushButton::clicked, this, &RelocationQuantityDialog::distributeEvenly);
    //connect(ui->btnOk, &QPushButton::clicked, this, &RelocationQuantityDialog::accept);
    //connect(ui->btnCancel, &QPushButton::clicked, this, &RelocationQuantityDialog::reject);
}

RelocationQuantityDialog::~RelocationQuantityDialog() {
    delete _ui;
}

void RelocationQuantityDialog::setRows(const QVector<RelocationQuantityRow>& rows,
                                       int planned,
                                       int selectedFromSources)
{
    _model.rows = rows;
    _plannedQuantity = planned;
    _selectedFromSources = selectedFromSources;

    // 🔹 Címkék és értékek beállítása a mód alapján
    if (_mode == QuantityDialogMode::Source) {
        _ui->labelPlannedCaption->setText("Mozgatandó");
        _ui->labelPlannedValue->setText(QString::number(_plannedQuantity));
        _ui->labelPlannedCaption->show();
        _ui->labelPlannedValue->show();

        _ui->labelSelectedCaption->setText("Kiválasztva");
        _ui->labelSelectedValue->show();
        _ui->labelSelectedCaption->show();
    }
    else if (_mode == QuantityDialogMode::Target) {
        _ui->labelPlannedCaption->setText("Elosztandó");
        _ui->labelPlannedValue->setText(QString::number(_selectedFromSources));
        _ui->labelPlannedCaption->show();
        _ui->labelPlannedValue->show();

        _ui->labelSelectedCaption->setText("Elosztva");
        _ui->labelSelectedValue->show();
        _ui->labelSelectedCaption->show();
    }
    else { // Both
        _ui->labelPlannedCaption->setText("Mozgatandó");
        _ui->labelPlannedValue->setText(QString::number(_plannedQuantity));
        _ui->labelPlannedCaption->show();
        _ui->labelPlannedValue->show();

        _ui->labelSelectedCaption->setText("Kiválasztva");
        _ui->labelSelectedValue->show();
        _ui->labelSelectedCaption->show();
    }

    // 🔹 Táblázat feltöltése
    auto* table = _ui->tableQuantities;
    table->clearContents();
    table->setRowCount(rows.size());
    table->horizontalHeader()->setStretchLastSection(true);

    for (int i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];

        table->setItem(i, 0, new QTableWidgetItem(r.storageName));

        auto* availItem = new QTableWidgetItem(QString::number(r.isTarget ? 0 : r.available));
        availItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        table->setItem(i, 1, availItem);

        auto* spin = new QSpinBox(table);
        spin->setMinimum(0);
        spin->setMaximum(r.isTarget ? 999999 : r.available);
        spin->setValue(r.selected);
        spin->setProperty("rowIndex", i);
        spin->setProperty("isTarget", r.isTarget);

        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int){
            updateSummary();
        });

        table->setCellWidget(i, 2, spin);
    }

    updateSummary();
}


QVector<RelocationQuantityRow> RelocationQuantityDialog::getRows() const {
    QVector<RelocationQuantityRow> out;
    auto* table = _ui->tableQuantities;
    out.reserve(table->rowCount());

    for (int i = 0; i < table->rowCount(); ++i) {
        RelocationQuantityRow r;
        r.storageName = table->item(i, 0)->text();
        r.available   = table->item(i, 1)->text().toInt();

        if (auto* w = table->cellWidget(i, 2)) {
            if (auto* spin = qobject_cast<QSpinBox*>(w)) {
                r.selected = spin->value();
                r.isTarget = spin->property("isTarget").toBool();
            }
        }
        out.append(r);
    }
    return out;
}


void RelocationQuantityDialog::updateSummary() {
    auto* table = _ui->tableQuantities;
    _totalSelectedFromSources = 0;
    _totalDistributedToTargets = 0;

    for (int i = 0; i < table->rowCount(); ++i) {
        if (auto* w = table->cellWidget(i, 2)) {
            if (auto* spin = qobject_cast<QSpinBox*>(w)) {
                bool isTarget = spin->property("isTarget").toBool();
                if (isTarget) _totalDistributedToTargets += spin->value();
                else          _totalSelectedFromSources += spin->value();
            }
        }
    }

    _ui->labelPlannedValue->setText(QString::number(_plannedQuantity));
    _ui->labelSelectedValue->setText(QString::number(_totalSelectedFromSources));

    if (_totalSelectedFromSources == _plannedQuantity)
        _ui->labelSelectedValue->setStyleSheet("color: green; font-weight: bold; font-size: 18pt;");
    else if (_totalSelectedFromSources < _plannedQuantity)
        _ui->labelSelectedValue->setStyleSheet("color: orange; font-weight: bold; font-size: 18pt;");
    else
        _ui->labelSelectedValue->setStyleSheet("color: red; font-weight: bold; font-size: 18pt;");

    // Állapotjelzés a mód alapján
    if (_mode == QuantityDialogMode::Source) {
        if (_totalSelectedFromSources == _plannedQuantity) {
            _ui->labelStatus->setStyleSheet("color: green;");
            _ui->labelStatus->setText("✔ Forrás rendben, finalizálható.");
        } else {
            _ui->labelStatus->setStyleSheet("color: orange;");
            _ui->labelStatus->setText("⚠ A forrásból felvett mennyiség nem egyezik az igénnyel.");
        }
    }
    else if (_mode == QuantityDialogMode::Target) {
        _totalSelectedFromSources = _selectedFromSources;

        _ui->labelPlannedValue->setText(QString::number(_selectedFromSources));
        _ui->labelSelectedValue->setText(QString::number(_totalDistributedToTargets));

        if (_totalDistributedToTargets == _selectedFromSources) {
            _ui->labelSelectedValue->setStyleSheet("color: green; font-weight: bold; font-size: 18pt;");
            _ui->labelStatus->setStyleSheet("color: green;");
            _ui->labelStatus->setText("✔ Cél kiosztás rendben.");
        } else {
            _ui->labelSelectedValue->setStyleSheet("color: orange; font-weight: bold; font-size: 18pt;");
            _ui->labelStatus->setStyleSheet("color: orange;");
            _ui->labelStatus->setText("⚠ A célokra kiosztott mennyiség nem egyezik a forrással.");
        }
    }

    else { // Both
        if (_totalDistributedToTargets > _totalSelectedFromSources) {
            _ui->labelStatus->setStyleSheet("color: red;");
            _ui->labelStatus->setText("❌ A célokra többet osztottál, mint amit felvettél.");
        }
        else if (_totalSelectedFromSources != _plannedQuantity) {
            _ui->labelStatus->setStyleSheet("color: orange;");
            _ui->labelStatus->setText("⚠ A forrásból felvett mennyiség nem egyezik az igénnyel.");
        }
        else if (_totalDistributedToTargets != _totalSelectedFromSources) {
            _ui->labelStatus->setStyleSheet("color: orange;");
            _ui->labelStatus->setText("⚠ A célokra kiosztott mennyiség nem egyezik a forrással.");
        }
        else {
            _ui->labelStatus->setStyleSheet("color: green;");
            _ui->labelStatus->setText("✔ A beállítás érvényes, finalizálható.");
        }
    }

    validate();
}

void RelocationQuantityDialog::validate() {
    //_ui->btnOk->setEnabled(isValid()); // ha lesz OK gomb, ez aktiválható
}

bool RelocationQuantityDialog::isValid() const {
    if (_mode == QuantityDialogMode::Source)
        return (_totalSelectedFromSources == _plannedQuantity);

    if (_mode == QuantityDialogMode::Target)
        return (_totalDistributedToTargets == _totalSelectedFromSources);

    return (_totalSelectedFromSources == _plannedQuantity &&
            _totalDistributedToTargets == _totalSelectedFromSources);
}
