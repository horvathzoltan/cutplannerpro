#include <QMessageBox>

#include <model/registries/storageregistry.h>

#include "addstockdialog.h"
#include "ui_addstockdialog.h"
#include "model/registries/materialregistry.h"
#include "common/quantityparser.h"



AddStockDialog::AddStockDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddStockDialog)
    , current_entryId(QUuid::createUuid()) // 🔑 Automatikusan új UUID
{
    ui->setupUi(this);
    populateMaterialCombo();
    populateStorageCombo();
}

AddStockDialog::~AddStockDialog()
{
    delete ui;
}

void AddStockDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().readAll();
    ui->comboMaterial->clear();

    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.displayName(), m.id);
    }
}

QUuid AddStockDialog::selectedMaterialId() const {
    return ui->comboMaterial->currentData().toUuid();
}

void AddStockDialog::populateStorageCombo() {
    ui->comboStorage->clear();
    const auto& storages = StorageRegistry::instance().readAll();
    for (const auto& s : storages) {
        ui->comboStorage->addItem(s.name, s.id);
    }
}

QUuid AddStockDialog::selectedStorageId() const {
    return ui->comboStorage->currentData().toUuid();
}

int AddStockDialog::quantity() const {
    int value = 0;
    QuantityParser::Mode mode = QuantityParser::parse(ui->lineEditQuantity->text().trimmed(), value);

    if (mode == QuantityParser::Absolute)
        return value;
    else if (mode == QuantityParser::Relative)
        return currentQuantity + value;
    else
        return -1; // hibás input
}


QString AddStockDialog::comment() const {
    return ui->editComment->text().trimmed();
}

StockEntry AddStockDialog::getModel() const {   
    StockEntry entry;
    entry.entryId = current_entryId; //visszaállítjuk az aktuális entryId-t
    entry.storageId = current_storageId; // visszaállítjuk a tároló ID-t
    entry.materialId = selectedMaterialId();
    entry.storageId  = selectedStorageId();
    entry.quantity = quantity();
    entry.comment = comment(); // ha a modell már tartalmazza
    return entry;
}

void AddStockDialog::setModel(const StockEntry& entry) {

    current_entryId = entry.entryId;
    currentQuantity = entry.quantity; // Megőrizzük az eredeti quantityt
    current_storageId = entry.storageId; // Megőrizzük a tároló ID-t

    // anyag
    if (int idx = ui->comboMaterial->findData(entry.materialId); idx >= 0)
        ui->comboMaterial->setCurrentIndex(idx);

    // mennyiség
    ui->lineEditQuantity->setText(QString::number(currentQuantity));

    // komment
    ui->editComment->setText(entry.comment);

    // tároló
    if (int sidx = ui->comboStorage->findData(entry.storageId); sidx >= 0)
        ui->comboStorage->setCurrentIndex(sidx);
}

bool AddStockDialog::validateInputs() {
    if (selectedMaterialId().isNull()) {
        QMessageBox::warning(this, "Hiányzó adat", "Kérlek válassz anyagot.");
        return false;
    }

    if (selectedStorageId().isNull()) { // 🆕
        QMessageBox::warning(this, "Hiányzó tároló", "Kérlek válassz tárolót.");
        return false;
    }

    if (quantity() < 0) {
        QMessageBox::warning(this, "Hibás mennyiség", "A megadott mennyiség nem értelmezhető. Használj egész számot vagy relatív formátumot (+/-).");
        return false;
    }

    return true;
}

void AddStockDialog::accept() {
    if (!validateInputs())
        return;

    QDialog::accept();
}

// int AddStockDialog::parsedQuantityDelta(int& resultValue) const {
//     QString input = ui->lineEditQuantity->text().trimmed();

//     if (input.startsWith("+") || input.startsWith("-")) {
//         QRegularExpression re("^\\s*([+-])\\s*(\\d+)\\s*$");
//         QRegularExpressionMatch match = re.match(input);

//         if (match.hasMatch()) {
//             QString op = match.captured(1);
//             int delta = match.captured(2).toInt();

//             resultValue = (op == "+") ? delta : -delta;
//             return 1; // relatív delta
//         }

//         return -1; // hibás relatív formátum
//     }

//     // Ha nincs +/- prefix, akkor abszolút érték
//     bool ok = false;
//     int value = input.toInt(&ok);
//     if (ok) {
//         resultValue = value;
//         return 0; // abszolút érték
//     }

//     return -1; // sehogy se értelmezhető

// }
