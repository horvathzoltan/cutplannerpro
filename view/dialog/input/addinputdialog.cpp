#include "addinputdialog.h"

//#include "qpushbutton.h"
#include "../../../model/cutting/plan/request.h"
#include "ui_addinputdialog.h"
#include "materials/registry/material_registry.h"

#include <QMessageBox>

QString AddInputDialog::s_lastOwnerName;
QString AddInputDialog::s_lastExternalRef;
QUuid   AddInputDialog::s_lastMaterialId;   // ⬅️ ÚJ

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
    req.requestId = current_requestId; // ✅ ez volt a hiányzó láncszem
    // 🔗 Anyag ID kinyerése a comboBox-ból
    QVariant matData = ui->comboMaterial->currentData();
    if (matData.isValid())
        req.materialId = matData.toUuid();

    // 📏 Vágási hossz kiolvasása
    bool okLen = false;
    req.requiredLength = ui->editLength->text().toInt(&okLen);
    if (!okLen)
        req.requiredLength = -1; // Hibás hossz

    // 🔢 Darabszám
    req.quantity = quantity(); // <- ha már van quantity() metódusod

    // 👤 Megrendelő neve
    req.ownerName = ownerName(); // <- ha már van ownerName() getter

    // 🧾 Külső tételszám
    req.externalReference = externalReference(); // <- ha van ilyen getter

    return req;
}


bool AddInputDialog::validateInputs() {
    Cutting::Plan::Request req = getModel(); // <- új metódusod, lásd korábban

    QStringList errors = req.invalidReasons(); // <- centralizált validáció

    if (!errors.isEmpty()) {
        QMessageBox::warning(this,
                             "Adatellenőrzés",
                             "Kérlek javítsd az alábbi hibákat:\n\n" + errors.join("\n"));
        return false;
    }

    return true;
}


void AddInputDialog::accept() {
    if (!validateInputs())
        return;

    // Mentjük a legutóbbi értékeket
    s_lastOwnerName   = ui->editOwner->text().trimmed();
    s_lastExternalRef = ui->editReference->text().trimmed();
    s_lastMaterialId  = ui->comboMaterial->currentData().toUuid();   // ⬅️ ÚJ

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
}


