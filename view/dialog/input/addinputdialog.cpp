#include "addinputdialog.h"

//#include "qpushbutton.h"
#include "../../../model/cutting/plan/request.h"
#include "ui_addinputdialog.h"
#include "materials/registry/material_registry.h"

#include <QMessageBox>

QString AddInputDialog::s_lastOwnerName;
QString AddInputDialog::s_lastExternalRef;
QUuid   AddInputDialog::s_lastMaterialId;   // ⬅️ ÚJ
Subtype AddInputDialog::s_lastSubtype = Subtype::None;

AddInputDialog::AddInputDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddInputDialog)
    , current_requestId(QUuid::createUuid()) // 🔑 Automatikusan új UUID
{
    ui->setupUi(this);
    populateMaterialCombo();

    // Anyag ajánlása (ha volt korábbi)
    if (!s_lastMaterialId.isNull()) {
        int idx = ui->comboMaterial->findData(s_lastMaterialId);
        if (idx >= 0)
            ui->comboMaterial->setCurrentIndex(idx);
    }

    // Megrendelő név ajánlása
    if (!s_lastOwnerName.isEmpty())
        ui->editOwner->setText(s_lastOwnerName);

    // Külső tételszám ajánlása (+1)
    if (!s_lastExternalRef.isEmpty()) {
        bool ok = false;
        int num = s_lastExternalRef.toInt(&ok);
        if (ok)
            ui->editReference->setText(QString::number(num + 1));
    }

    // Ha mindkettő megvolt → fókusz a hossz mezőre
    if (!ui->editOwner->text().isEmpty() &&
        !ui->editReference->text().isEmpty())
    {
        ui->editLength->setFocus();
        ui->editLength->selectAll();
    }


    ui->editLength->setValidator(new QIntValidator(0, 100000, this));

    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int qty){
                if (qty == 1) {
                    ui->spinBox_left->setMaximum(1);
                    ui->spinBox_right->setMaximum(1);
                } else {
                    ui->spinBox_left->setMaximum(qty);
                    ui->spinBox_right->setMaximum(qty);
                }
            });

    QString bigArrows = R"(
QSpinBox::up-button {
    width: 24px;
    height: 18px;
}
QSpinBox::down-button {
    width: 24px;
    height: 18px;
}
)";

    ui->spinBox_left->setStyleSheet(bigArrows);
    ui->spinBox_right->setStyleSheet(bigArrows);
    ui->spinQuantity->setStyleSheet(bigArrows);

}

AddInputDialog::~AddInputDialog()
{
    delete ui;
}


void AddInputDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().readAll();

    ui->comboMaterial->clear();
    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.toDisplay(), m.id);
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

QString AddInputDialog::ownerName() const {
    return ui->editOwner->text().trimmed();
}

QString AddInputDialog::externalReference() const {
    return ui->editReference->text().trimmed();
}

Cutting::Plan::Request AddInputDialog::getModel() const {
    Cutting::Plan::Request req;
    req.requestId = current_requestId;

    // Anyag
    QVariant matData = ui->comboMaterial->currentData();
    if (matData.isValid())
        req.materialId = matData.toUuid();

    // Hossz
    bool okLen = false;
    req.requiredLength = ui->editLength->text().toInt(&okLen);
    if (!okLen)
        req.requiredLength = -1;

    // Darabszám
    req.quantity = ui->spinQuantity->value();

    // Megrendelő
    req.ownerName = ui->editOwner->text().trimmed();

    // Külső tételszám
    req.externalReference = ui->editReference->text().trimmed();

    // ⭐ J/B darabszámok
    req.leftCount  = ui->spinBox_left->value();
    req.rightCount = ui->spinBox_right->value();

    // ⭐ Altípus
    req.subtype = parseSubtypeFromRadioButtons();

    return req;
}



bool AddInputDialog::validateInputs() {

    // ⭐ J/B darabszám ellenőrzés
    int qty = ui->spinQuantity->value();
    int l   = ui->spinBox_left->value();
    int r   = ui->spinBox_right->value();

    if ((l + r != qty) && !(l==0 &&r ==0)) {
        QMessageBox::warning(this,
                             "Hibás J/B megadás",
                             "A balos és jobbos darabszám összege nem egyezik meg a teljes darabszámmal.");
        return false;
    }

    // ⭐ Altípushoz nem kell külön validáció (mindig van választás)

    // ⭐ A többi mező validálása
    Cutting::Plan::Request req = getModel();
    QStringList errors = req.invalidReasons();

    if (!errors.isEmpty()) {
        QMessageBox::warning(this,
                             "Adatellenőrzés",
                             "Kérlek javítsd az alábbi hibákat:\n\n" + errors.join("\n"));
        return false;
    }

    int len = ui->editLength->text().toInt();
    if (len < 100) {
        QMessageBox::warning(this,
                             "Hibás hossz",
                             "A vágási hossz nem lehet 100 mm alatt. "
                             "Ha tizedespontot írtál, javítsd ki egész számra.");
        return false;
    }

    if (len < 200) {
        QMessageBox::warning(this,
                             "Túl rövid darab",
                             "200 mm alatti darabot nem vágunk. "
                             "A gyorsdaraboló nem szalámiszeletelő.");
        return false;
    }

    return true;
}


Subtype AddInputDialog::parseSubtypeFromRadioButtons() const
{
    if (ui->radioButton_alap->isChecked())
        return Subtype::Alap;
    else if (ui->radioButton_rugos->isChecked())
        return Subtype::Rugos;
    else if (ui->radioButton_tetoteri->isChecked())
        return Subtype::Tetoteri;
    else
        return Subtype::None;
}

void AddInputDialog::accept() {
    if (!validateInputs())
        return;

    // Mentjük a legutóbbi értékeket
    s_lastOwnerName   = ui->editOwner->text().trimmed();
    s_lastExternalRef = ui->editReference->text().trimmed();
    s_lastMaterialId  = ui->comboMaterial->currentData().toUuid();   // ⬅️ ÚJ

    // ⭐ ÚJ: altípus mentése
    s_lastSubtype = parseSubtypeFromRadioButtons();

    QDialog::accept(); // csak ha minden oké
}

void AddInputDialog::setModel(const Cutting::Plan::Request& request) {
    current_requestId = request.requestId; // ⬅️ ID mentése

    // 🔗 Anyag beállítása comboBox-ban
    int index = ui->comboMaterial->findData(request.materialId);
    if (index >= 0)
        ui->comboMaterial->setCurrentIndex(index);

    // 📏 Hossz
    ui->editLength->setText(QString::number(request.requiredLength));

    // 🔢 Darabszám
    ui->spinQuantity->setValue(request.quantity);

    // 👤 Megrendelő
    ui->editOwner->setText(request.ownerName);

    // 🧾 Külső azonosító
    ui->editReference->setText(request.externalReference);

    // ⭐ J/B visszatöltés
    ui->spinBox_left->setValue(request.leftCount);
    ui->spinBox_right->setValue(request.rightCount);

    // ⭐ Altípus visszatöltés
    switch (request.subtype) {
    case Subtype::Alap:     ui->radioButton_alap->setChecked(true); break;
    case Subtype::Rugos:    ui->radioButton_rugos->setChecked(true); break;
    case Subtype::Tetoteri: ui->radioButton_tetoteri->setChecked(true); break;
    default:                ui->radioButton_nincs->setChecked(true); break;
    }

}


