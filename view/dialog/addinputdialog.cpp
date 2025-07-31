#include "addinputdialog.h"

//#include "qpushbutton.h"
#include "model/cuttingrequest.h"
#include "ui_addinputdialog.h"
#include "model/registries/materialregistry.h"

#include <QMessageBox>

AddInputDialog::AddInputDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddInputDialog)
    , currentRequestId(QUuid::createUuid()) // 🔑 Automatikusan új UUID
{
    ui->setupUi(this);
    populateMaterialCombo();

    // ui->comboCategory->addItem("RollerTube");
    // ui->comboCategory->addItem("BottomBar");

    // // Ok gomb disable ha nincs értelmes adat
    // connect(ui->editLength, &QLineEdit::textChanged, this, [=]() {
    //     ui->buttonBox->button(QDialogButtonBox::Ok)
    //         ->setEnabled(!ui->editLength->text().isEmpty());
    // });
}

AddInputDialog::~AddInputDialog()
{
    delete ui;
}


void AddInputDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().all();

    ui->comboMaterial->clear();
    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.displayName(), m.id);
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

CuttingRequest AddInputDialog::getModel() const {
    CuttingRequest req;
    req.requestId = currentRequestId; // ✅ ez volt a hiányzó láncszem
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
    CuttingRequest req = getModel(); // <- új metódusod, lásd korábban

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

    QDialog::accept(); // csak ha minden oké
}

void AddInputDialog::setModel(const CuttingRequest& request) {
    currentRequestId = request.requestId; // ⬅️ ID mentése

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


