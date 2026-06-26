#include "addwastedialog.h"
#include "service/cutting/result/leftoversourceutils.h"
#include "ui_addwastedialog.h"
#include "materials/registry/material_registry.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"
#include <QMessageBox>
#include <common/identifierutils.h>
#include "settings/settingsmanager.h"
#include <model/registries/leftoverstockregistry.h>
#include <model/registries/storageregistry.h>

QUuid AddWasteDialog::s_lastMaterialId;
int   AddWasteDialog::s_lastLength = 0;
QUuid AddWasteDialog::s_lastStorageId;
QString AddWasteDialog::s_lastBarcode;

bool AddWasteDialog::s_lastRepeat = false;

AddWasteDialog::AddWasteDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddWasteDialog)
    , current_entryId(QUuid::createUuid())
{
    ui->setupUi(this);
    populateMaterialCombo();

    connect(ui->btn_MaterialSearch, &QPushButton::clicked, this, [this]() {
        MaterialSearchDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted) {
            auto sel = dlg.selection();

            // A kiválasztott anyag beállítása a comboBox-ban
            for (int i = 0; i < ui->comboMaterial->count(); ++i) {
                QUuid id = ui->comboMaterial->itemData(i).toUuid();
                if (id == sel.id) {
                    ui->comboMaterial->setCurrentIndex(i);
                    break;
                }
            }
        }
    });

    populateStorageCombo();

    // Anyag ajánlása
    if (!s_lastMaterialId.isNull()) {
        int idx = ui->comboMaterial->findData(s_lastMaterialId);
        if (idx >= 0)
            ui->comboMaterial->setCurrentIndex(idx);
    }

    // Hossz ajánlása
    if (s_lastLength > 0)
        ui->editLength->setText(QString::number(s_lastLength));

    // Tárhely ajánlása
    if (!s_lastStorageId.isNull()) {
        int sidx = ui->comboStorage->findData(s_lastStorageId);
        if (sidx >= 0)
            ui->comboStorage->setCurrentIndex(sidx);
    }

    // Prefix-aware ajánlás (pl. RSM-129 → RSM-130, maki-55 → maki-56)
    QString bc;
    if (!s_lastBarcode.isEmpty()) {
        QRegularExpression re("^(.*?)(\\d+)$");
        QRegularExpressionMatch m = re.match(s_lastBarcode);
        if (m.hasMatch()) {
            QString prefix = m.captured(1);
            QString numStr = m.captured(2);
            int num = numStr.toInt();
            int inc = num + 1;
            QString padded = QString("%1").arg(inc, numStr.length(), 10, QChar('0'));
            bc = prefix + padded;
        }
    }
    ui->editBarcode->setText(bc);

    ui->chk_Repeat->setChecked(s_lastRepeat);
}


AddWasteDialog::~AddWasteDialog()
{
    delete ui;
}

void AddWasteDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().readAll();
    ui->comboMaterial->clear();

    for (const auto& m : registry) {
        ui->comboMaterial->addItem(m.toDisplay(), m.id);
    }
}

QUuid AddWasteDialog::selectedMaterialId() const {
    return ui->comboMaterial->currentData().toUuid();
}

QString AddWasteDialog::barcode() const {
    return ui->editBarcode->text().trimmed().toUpper();
}

int AddWasteDialog::availableLength() const {
    return ui->editLength->text().toInt(); // validáció később
}

// QString AddWasteDialog::comment() const {
//     return ui->editComment->text().trimmed();
// }

Cutting::Result::LeftoverSource AddWasteDialog::source() const {
    QString a = ui->editSourceValue->text();
    Cutting::Result::LeftoverSource b = LeftoverSourceUtils::fromString(a);
    return b;
}

LeftoverStockEntry AddWasteDialog::getModel() const {
    LeftoverStockEntry entry;
    entry.entryId = current_entryId;

    entry.materialId = selectedMaterialId();
    entry.availableLength_mm = availableLength();
    entry.barcode = barcode();
    entry.source = source();
    entry.optimizationId = std::nullopt;

    // 🆕 STORAGE
    QUuid sid = selectedStorageId();
    entry.storageId = sid.isNull() ? current_storageId : sid;

    zInfo(QString("USER CREATED LEFTOVER: entryId=%1, material=%2, length=%3, barcode=%4")
              .arg(entry.entryId.toString())
              .arg(entry.materialId.toString())
              .arg(entry.availableLength_mm)
              .arg(entry.barcode));

    return entry;
}

void AddWasteDialog::setModel(const LeftoverStockEntry& entry) {
    current_entryId = entry.entryId; // ha szerkesztés módban vagyunk
    current_storageId  = entry.storageId;

    shadowManualCounter = SettingsManager::instance().peekManualLeftoverCounter(); // shadow counter init

    QString bc = entry.barcode.trimmed().toUpper();

    // 🔥 Ha a modelben nincs barcode → generáljunk egyet
    if (bc.isEmpty()) {

        // 1) Prefix-aware +1 (pl. RSM-129 → RSM-130, maki-55 → maki-56)
        QRegularExpression re("^(.*?)(\\d+)$");
        QRegularExpressionMatch m = re.match(s_lastBarcode);
        if (m.hasMatch()) {
            QString prefix = m.captured(1);
            QString numStr = m.captured(2);
            int num = numStr.toInt();
            int inc = num + 1;
            QString padded = QString("%1").arg(inc, numStr.length(), 10, QChar('0'));
            bc = prefix + padded;
        } else {
            // 2) Shadow counter fallback → csak ha nincs prefix-match
            int next = shadowManualCounter;
            bc = IdentifierUtils::makeManualLeftoverId(next);

            while (LeftoverStockRegistry::instance().existsBarcode(bc)) {
                next++;
                bc = IdentifierUtils::makeManualLeftoverId(next);
            }

            shadowManualCounter = next;
        }

        ui->editBarcode->setText(bc);
    }

    ui->editBarcode->setText(bc);

    ui->editLength->setText(QString::number(entry.availableLength_mm));

    auto leftoverTxt = LeftoverSourceUtils::toString(entry.source);
    ui->editSourceValue->setText(leftoverTxt);

    int index = ui->comboMaterial->findData(entry.materialId);
    if (index >= 0)
        ui->comboMaterial->setCurrentIndex(index);

    if (!entry.storageId.isNull()) {
        int sidx = ui->comboStorage->findData(entry.storageId);
        if (sidx >= 0)
            ui->comboStorage->setCurrentIndex(sidx);
    }
}

bool AddWasteDialog::validateInputs() {
    if (selectedMaterialId().isNull()) {
        QMessageBox::warning(this, "Anyag hiányzik", "Válassz ki egy anyagot!");
        return false;
    }

    if (availableLength() <= 0) {
        QMessageBox::warning(this, "Hibás hossz", "Az elérhető hossz csak pozitív szám lehet!");
        return false;
    }

    // ❗ Hullóknál a storage opcionális → NEM ellenőrizzük

    QString bc = barcode().trimmed();
    if (bc.isEmpty()) {
        QMessageBox::warning(this, "Hiányzó vonalkód", "Adj meg egy érvényes vonalkódot!");
        return false;
    }

    if (!validateBarcodeFormat(bc)) {
        QMessageBox::warning(this,
                             "Hibás vonalkód",
                             "A vonalkód csak ASCII 32–126 karaktereket tartalmazhat, "
                             "Ékezetes betűk nem engedélyezettek.");
        return false;
    }

    // 🔥 Barcode egyediség ellenőrzése
    if (LeftoverStockRegistry::instance().existsBarcode(bc, current_entryId)) {
        QMessageBox::warning(this,
                             "Duplikált vonalkód",
                             "Ez a vonalkód már létezik a hullók között!");
        return false;
    }

    return true;
}

void AddWasteDialog::accept() {
    if (!validateInputs())
        return;

    s_lastMaterialId = selectedMaterialId();

    bool ok = false;
    int len = ui->editLength->text().toInt(&ok);
    if (ok && len > 0)
        s_lastLength = len;

    QUuid sid = selectedStorageId();
    if (!sid.isNull())
        s_lastStorageId = sid;

    // Vonalkód mentése → következő ajánláshoz
    QString bc = barcode();
    if (!bc.isEmpty())
        s_lastBarcode = bc;

    // shadow counter commit → csak RSM prefix esetén
    if (bc.startsWith("RSM"))
        SettingsManager::instance().commitManualLeftoverCounter(shadowManualCounter);

    s_lastRepeat = ui->chk_Repeat->isChecked();
    QDialog::accept();
}


void AddWasteDialog::populateStorageCombo() {
    ui->comboStorage->clear();
    const auto& storages = StorageRegistry::instance().readAll();
    for (const auto& s : storages) {
        ui->comboStorage->addItem(s.name, s.id);
    }
}

QUuid AddWasteDialog::selectedStorageId() const {
    return ui->comboStorage->currentData().toUuid();
}

bool AddWasteDialog::shouldRepeat()
{
    return ui->chk_Repeat->isChecked();
}

bool AddWasteDialog::validateBarcodeFormat(const QString& bc) const
{
    if (bc.isEmpty())
        return false;

    // Code128: ASCII 32–126
    for (QChar c : bc) {
        ushort u = c.unicode();
        if (u < 32 || u > 126)
            return false;
    }

    return true;
}
