#include "addstockdialog.h"
#include "ui_addstockdialog.h"
#include "model/registries/materialregistry.h"

#include <QMessageBox>

AddStockDialog::AddStockDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddStockDialog)
    , current_entryId(QUuid::createUuid()) // 🔑 Automatikusan új UUID
{
    ui->setupUi(this);
    populateMaterialCombo();
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

int AddStockDialog::quantity() const {
    int value = 0;
    int mode = parsedQuantityDelta(value);

    if (mode == 0)
        return value;
    else if (mode == 1)
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
    entry.quantity = quantity();
    return entry;
}

void AddStockDialog::setModel(const StockEntry& entry) {

    current_entryId = entry.entryId;
    currentQuantity = entry.quantity; // Megőrizzük az eredeti quantityt
    current_storageId = entry.storageId; // Megőrizzük a tároló ID-t

    int index = ui->comboMaterial->findData(entry.materialId);
    if (index >= 0)
        ui->comboMaterial->setCurrentIndex(index);

    ui->lineEditQuantity->setText(QString::number(currentQuantity));
    ui->editComment->setText(""); // opcionálisan beállítható
}

bool AddStockDialog::validateInputs() {
    if (selectedMaterialId().isNull()) {
        QMessageBox::warning(this, "Hiányzó adat", "Kérlek válassz anyagot.");
        return false;
    }

    if (quantity() < 0) {
        QMessageBox::warning(this, "Hibás mennyiség", "A mennyiségnek pozitívnak kell lennie.");
        return false;
    }

    return true;
}

void AddStockDialog::accept() {
    if (!validateInputs())
        return;

    QDialog::accept();
}

int AddStockDialog::parsedQuantityDelta(int& resultValue) const {
    QString input = ui->lineEditQuantity->text().trimmed();

    if (input.startsWith("+") || input.startsWith("-")) {
        QRegularExpression re("^\\s*([+-])\\s*(\\d+)\\s*$");
        QRegularExpressionMatch match = re.match(input);

        if (match.hasMatch()) {
            QString op = match.captured(1);
            int delta = match.captured(2).toInt();

            resultValue = (op == "+") ? delta : -delta;
            return 1; // relatív delta
        }

        return -1; // hibás relatív formátum
    }

    // Ha nincs +/- prefix, akkor abszolút érték
    bool ok = false;
    int value = input.toInt(&ok);
    if (ok) {
        resultValue = value;
        return 0; // abszolút érték
    }

    return -1; // sehogy se értelmezhető

}
