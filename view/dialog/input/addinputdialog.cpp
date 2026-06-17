#include "addinputdialog.h"

//#include "qpushbutton.h"
#include "../../../model/cutting/plan/request.h"
#include "ui_addinputdialog.h"
#include "materials/registry/material_registry.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"

#include <QKeyEvent>
#include <QMessageBox>

QString AddInputDialog::s_lastOwnerName;
QString AddInputDialog::s_lastExternalRef;
QUuid   AddInputDialog::s_lastMaterialId;   // ⬅️ ÚJ
Subtype AddInputDialog::s_lastSubtype = Subtype::None;
QDate AddInputDialog::s_lastDueDate = QDate();   // default: invalid
bool AddInputDialog::s_lastRepeat = false;

AddInputDialog::AddInputDialog(QWidget *parent, DialogMode mode)
    : QDialog(parent)
    , ui(new Ui::AddInputDialog)
    , current_requestId(QUuid::createUuid())
{
    _mode = mode;
    _shiftEnterAccepted = false;

    ui->setupUi(this);

    ui->editLength->installEventFilter(this);

    // ⭐ Slider UI tuning
    ui->sliderHandler->setStyleSheet(R"(
QSlider::groove:horizontal {
    height: 10px;
    background: #d0d0d0;
    border-radius: 5px;
}

QSlider::handle:horizontal {
    background: #4a90e2;
    width: 22px;
    height: 22px;
    margin: -6px 0;
    border-radius: 11px;
}

QSlider::sub-page:horizontal {
    background: #4a90e2;
    border-radius: 5px;
}

QSlider::add-page:horizontal {
    background: #b0b0b0;
    border-radius: 5px;
}
)");

    // ⭐ Focus highlight (globális dialog szintű)
    this->setStyleSheet(R"(
QLineEdit:focus,
QComboBox:focus,
QSpinBox:focus,
QSlider:focus,
QRadioButton:focus {
    outline: 2px solid #4a90e2;
    outline-offset: 2px;
}
QLineEdit[hasError="true"] {
    border: 2px solid red;
}
)");

    //ui->editDueDate->setDate(QDate::currentDate());

    populateMaterialCombo();

    // ⭐ Altípus visszatöltése
    switch (s_lastSubtype) {
    case Subtype::Alap:     ui->radioButton_alap->setChecked(true); break;
    case Subtype::Rugos:    ui->radioButton_rugos->setChecked(true); break;
    case Subtype::Tetoteri: ui->radioButton_tetoteri->setChecked(true); break;
    default:                ui->radioButton_nincs->setChecked(true); break;
    }

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

    if (_mode == DialogMode::Create) {
        if (s_lastDueDate.isValid())
            ui->editDueDate->setDate(s_lastDueDate);
        else
            ui->editDueDate->setDate(QDate::currentDate());
    }

    if (mode == DialogMode::Update) {
        ui->chk_Repeat->setChecked(false);
        ui->chk_Repeat->setEnabled(false);
        ui->chk_Repeat->setVisible(false);
    } else {
        ui->chk_Repeat->setChecked(s_lastRepeat);
        ui->chk_Repeat->setVisible(true);
    }


    if (!ui->editOwner->text().isEmpty() &&
        !ui->editReference->text().isEmpty())
    {
        ui->editLength->setFocus();
        ui->editLength->selectAll();
    }

    // ⭐ Realtime hibajelzés a hossz mezőben
    connect(ui->editLength, &QLineEdit::textChanged, this, [this](const QString &text) {
        QRegularExpression re("^[1-9][0-9]{0,4}$");
        bool ok = re.match(text).hasMatch();

        if (!ok) {
            ui->editLength->setProperty("hasError", true);
            ui->editLength->setToolTip("A hossz csak 1–99999 közötti egész szám lehet.");
        } else {
            ui->editLength->setProperty("hasError", false);
            ui->editLength->setToolTip("");
        }
        ui->editLength->style()->unpolish(ui->editLength);
        ui->editLength->style()->polish(ui->editLength);

    });

    // qty változás → handler‑UI váltás
    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged),
            this, &AddInputDialog::onQuantityChanged);

    // slider változás → label frissítés
    connect(ui->sliderHandler, &QSlider::valueChanged,
            this, &AddInputDialog::updateSliderLabels);

    // Fókusz fix
    QTimer::singleShot(0, this, [this]() {
        if (ui->editOwner->text().isEmpty()) {
            ui->editOwner->setFocus();
            return;
        }

        if (ui->editReference->text().isEmpty()) {
            ui->editReference->setFocus();
            return;
        }

        if (!ui->editLength->text().isEmpty()) {
            ui->editLength->setFocus();
            ui->editLength->selectAll();
            return;
        }

        ui->editLength->setFocus();
    });

    // induló állapot
    //onQuantityChanged(ui->spinQuantity->value());

    // connect(ui->btn_MaterialSearch, &QPushButton::clicked,
    //         this, &AddInputDialog::on_btn_MaterialSearch_clicked);
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
    const int totalPieces = ui->spinQuantity->value();
    req.quantity = totalPieces;

    // Megrendelő
    req.ownerName = ui->editOwner->text().trimmed();

    // Külső tételszám
    req.externalReference = ui->editReference->text().trimmed();

    // ⭐ J/B darabszámok
    if (totalPieces == 1) {
        req.leftCount  = ui->radioLeft->isChecked() ? 1 : 0;
        req.rightCount = ui->radioLeft->isChecked() ? 0 : 1;
    } else {
        int left = ui->sliderHandler->value();
        req.leftCount  = left;
        req.rightCount = totalPieces - left;
    }


    // Altípus
    req.subtype = parseSubtypeFromRadioButtons();

    req.dueDate = ui->editDueDate->date();


    return req;
}



bool AddInputDialog::validateInputs() {

    Cutting::Plan::Request req = getModel();
    const int totalPieces = req.quantity;
    const int l = req.leftCount;
    const int r = req.rightCount;

    // J/B darabszám ellenőrzés
    if ((l + r != totalPieces) && !(l == 0 && r == 0)) {
        QMessageBox::warning(this,
                             "Hibás J/B megadás",
                             "A balos és jobbos darabszám összege nem egyezik meg a teljes darabszámmal.");
        return false;
    }

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

    if (!req.dueDate.isValid()) {
        QMessageBox::warning(this, "Hibás dátum", "A határidő érvénytelen.");
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
    s_lastDueDate = ui->editDueDate->date();
    s_lastRepeat = ui->chk_Repeat->isChecked();

    QDialog::accept(); // csak ha minden oké
}

void AddInputDialog::setModel(const Cutting::Plan::Request& request) {
    current_requestId = request.requestId;

    int index = ui->comboMaterial->findData(request.materialId);
    if (index >= 0)
        ui->comboMaterial->setCurrentIndex(index);

    ui->editLength->setText(QString::number(request.requiredLength));
    ui->spinQuantity->setValue(request.quantity);
    ui->editOwner->setText(request.ownerName);
    ui->editReference->setText(request.externalReference);

    // handler UI
    onQuantityChanged(request.quantity); // beállítja a módot + slider range-et

    // ⭐ J/B visszatöltés
    onQuantityChanged(request.quantity);

    if (request.quantity == 1) {
        ui->radioLeft->setChecked(request.leftCount == 1);
        ui->radioRight->setChecked(request.rightCount == 1);
    } else {
        ui->sliderHandler->setValue(request.leftCount);
        updateSliderLabels();
    }

    // Altípus
    switch (request.subtype) {
    case Subtype::Alap:     ui->radioButton_alap->setChecked(true); break;
    case Subtype::Rugos:    ui->radioButton_rugos->setChecked(true); break;
    case Subtype::Tetoteri: ui->radioButton_tetoteri->setChecked(true); break;
    default:                ui->radioButton_nincs->setChecked(true); break;
    }

    ui->editDueDate->setDate(request.dueDate);

}

bool AddInputDialog::shouldRepeat()
{
    return ui->chk_Repeat->isChecked();
}

void AddInputDialog::onQuantityChanged(int totalPieces)
{
    if (totalPieces <= 1) {
        ui->stackHandlerSide->setCurrentIndex(0);   // radio mód
        ui->radioLeft->setChecked(true);            // default: balos
    } else {
        ui->stackHandlerSide->setCurrentIndex(1);   // slider mód
        ui->sliderHandler->setMinimum(0);
        ui->sliderHandler->setMaximum(totalPieces);
        ui->sliderHandler->setValue(totalPieces);   // default: minden bal
    }
    updateSliderLabels();
}

void AddInputDialog::updateSliderLabels()
{
    int totalPieces = ui->spinQuantity->value();
    int left        = ui->sliderHandler->value();
    int right       = totalPieces - left;

    ui->labelLeftValue->setText(QString::number(left));
    ui->labelRightValue->setText(QString::number(right));
}

void AddInputDialog::keyPressEvent(QKeyEvent *e)
{
    // Shift+Enter → OK + új tétel
    // if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) &&
    //     (e->modifiers() & Qt::ShiftModifier))
    // {
    //     if (validateInputs()) {
    //         _shiftEnterAccepted = true;
    //         accept();
    //     }
    //     return;
    // }

    // Enter → OK, de csak ha nem QLineEdit-ben vagyunk
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        QWidget *w = focusWidget();

        if (qobject_cast<QLineEdit*>(w)) {
            focusNextChild();
            return;
        }

        if (validateInputs()) {
            accept();
        }
        return;
    }

    // Esc → Cancel
    if (e->key() == Qt::Key_Escape) {
        reject();
        return;
    }

    QDialog::keyPressEvent(e);
}

//⭐ Length mező automatikus kijelölése
bool AddInputDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->editLength && event->type() == QEvent::FocusIn) {
        ui->editLength->selectAll();
    }
    return QDialog::eventFilter(obj, event);
}


void AddInputDialog::on_btn_MaterialSearch_clicked()
{
    MaterialSearchDialog dlg(this);

    if (dlg.exec() != QDialog::Accepted)
        return;

    MaterialSelection sel = dlg.selection();
    if (sel.id.isNull())
        return;

    // A comboMaterial frissítése
    int idx = ui->comboMaterial->findData(sel.id);
    if (idx >= 0) {
        ui->comboMaterial->setCurrentIndex(idx);
    } else {
        // Ha valamiért nincs benne, hozzáadjuk
        ui->comboMaterial->addItem(sel.master.toDisplay(), sel.id);
        ui->comboMaterial->setCurrentIndex(ui->comboMaterial->count() - 1);
    }
}

void AddInputDialog::reject() {
    // Cancel → reset repeat
    s_lastRepeat = false;
    QDialog::reject();
}
