#include "addinputdialog.h"

#include "../../../model/cutting/plan/request.h"
#include "ui_addinputdialog.h"
#include "materials/registry/material_registry.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"

#include <QCompleter>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <common/eventlogger.h>

QString AddInputDialog::s_lastExternalRef;
QMap<QString, AddInputDialog::RequestContext> AddInputDialog::_contexts;
QSet<QString> AddInputDialog::s_ownerCache;

bool AddInputDialog::s_lastRepeat = false;

AddInputDialog::AddInputDialog(QWidget *parent, DialogMode mode)
    : QDialog(parent)
    , ui(new Ui::AddInputDialog)
    , current_requestId(QUuid::createUuid())
{
    if (_contexts.isEmpty()) {
        loadContextMap();
    }

    if (s_ownerCache.isEmpty()) {
        loadOwnerCache();
    }

    _mode = mode;
    _shiftEnterAccepted = false;

    ui->setupUi(this);

    ui->editLength->installEventFilter(this);

    auto completer = new QCompleter(s_ownerCache.values(), this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->editOwner->setCompleter(completer);

    // ⭐ Slider UI tuning
//     ui->sliderHandler->setStyleSheet(R"(
// QSlider::groove:horizontal {
//     height: 10px;
//     background: #d0d0d0;
//     border-radius: 5px;
// }

// QSlider::handle:horizontal {
//     background: #4a90e2;
//     width: 22px;
//     height: 22px;
//     margin: -6px 0;
//     border-radius: 11px;
// }

// QSlider::sub-page:horizontal {
//     background: #4a90e2;
//     border-radius: 5px;
// }

// QSlider::add-page:horizontal {
//     background: #b0b0b0;
//     border-radius: 5px;
// }
// )");

//     // ⭐ Focus highlight (globális dialog szintű)
//     this->setStyleSheet(R"(
// QLineEdit:focus,
// QComboBox:focus,
// QSpinBox:focus,
// QSlider:focus,
// QRadioButton:focus {
//     outline: 2px solid #4a90e2;
//     outline-offset: 2px;
// }
// QLineEdit[hasError="true"] {
//     border: 2px solid red;
// }
// )");



    populateMaterialCombo();

    // Külső tételszám ajánlása (+1)
    // if (!s_lastExternalRef.isEmpty()) {
    //     bool ok = false;
    //     int num = s_lastExternalRef.toInt(&ok);
    //     if (ok)
    //         ui->editReference->setText(QString::number(num + 1));
    // }

    // Ha van context az előző tételszámhoz, ajánljuk a megrendelőt / dátumot / altípust / oldalt
    // auto it = _contexts.find(s_lastExternalRef);
    // if (it != _contexts.end()) {
    //     const RequestContext& ctx = it.value();

    //     ui->editOwner->setText(ctx.ownerName);
    //     ui->editDueDate->setDate(ctx.dueDate);

    //     switch (ctx.subtype) {
    //     case Subtype::Alap:     ui->radioButton_alap->setChecked(true); break;
    //     case Subtype::Rugos:    ui->radioButton_rugos->setChecked(true); break;
    //     case Subtype::Tetoteri: ui->radioButton_tetoteri->setChecked(true); break;
    //     default:                ui->radioButton_nincs->setChecked(true); break;
    //     }

    //     if (ctx.side == HandlerSide::Left)
    //         ui->radioLeft->setChecked(true);
    //     else if (ctx.side == HandlerSide::Right)
    //         ui->radioRight->setChecked(true);

    //     if (!ctx.defaultMaterialId.isNull()) {
    //         int idx = ui->comboMaterial->findData(ctx.defaultMaterialId);
    //         if (idx >= 0)
    //             ui->comboMaterial->setCurrentIndex(idx);
    //     }
    // } else {
    //     // ha nincs context, dueDate default: ma
    //     ui->editDueDate->setDate(QDate::currentDate());
    // }

    // ajánlott tételszám = last + 1
    QString nextRef;
    if (!s_lastExternalRef.isEmpty()) {
        bool ok = false;
        int num = s_lastExternalRef.toInt(&ok);
        if (ok)
            nextRef = QString::number(num + 1);
    }
    ui->editReference->setText(nextRef);

    // ha volt előző tételszám → töltsük vissza a contextet
//    auto it = _contexts.find(s_lastExternalRef);
//    if (it != _contexts.end()) {
//        applyContextToWidgets(it.value());
//    }

    auto it = _contexts.find(nextRef);
    if (it != _contexts.end()) {
        applyContextToWidgets(it.value());
    } else{
        auto it = _contexts.find(s_lastExternalRef);
        if (it != _contexts.end()) {
            applyContextToWidgets(it.value());
        }
    }

    // új tételszám → reset kritikus mezők
    ui->editLength->clear();
    ui->spinQuantity->setValue(1);

    // dueDate → holnap
    //ui->editDueDate->setDate(QDate::currentDate().addDays(1));
    ui->editDueDate-> clear();//setDate(QDate::currentDate());

    ui->editOwner->clear();

    // új tételszám → editable
    setContextEditable(true);

    if (mode == DialogMode::Update) {
        ui->chk_Repeat->setChecked(false);
        ui->chk_Repeat->setEnabled(false);
        ui->chk_Repeat->setVisible(false);
    } else {
        ui->chk_Repeat->setChecked(s_lastRepeat);
        ui->chk_Repeat->setVisible(true);
    }


    // if (!ui->editOwner->text().isEmpty() &&
    //     !ui->editReference->text().isEmpty())
    // {
    //     ui->editLength->setFocus();
    //     ui->editLength->selectAll();
    // }

    // ⭐ Realtime hibajelzés a hossz mezőben
    // connect(ui->editReference, &QLineEdit::textChanged,
    //         this, [this](const QString& ref) {

    //             auto it = _contexts.find(ref.trimmed());
    //             if (it == _contexts.end()) {
    //                 setContextEditable(true);
    //                 return;
    //             }

    //             applyContextToWidgets(it.value());
    //             setContextEditable(false);
    //         });

    connect(ui->editReference, &QLineEdit::textChanged,
            this, [this](const QString& ref) {

                QString trimmed = ref.trimmed();

                // 1) LÉTEZŐ TÉTELSZÁM → context + readonly
                auto it = _contexts.find(trimmed);
                if (it != _contexts.end()) {
                    applyContextToWidgets(it.value());
                    setContextEditable(false);
                    return;
                }

                // 2) ÚJ TÉTELSZÁM → last context + reset + editable
                auto last = _contexts.find(s_lastExternalRef);
                if (last != _contexts.end()) {
                    applyContextToWidgets(last.value());
                }

                // reset kritikus mezők
                ui->editLength->clear();
                ui->spinQuantity->setValue(1);

                // dueDate → holnap
                ui->editDueDate->setDate(QDate::currentDate().addDays(1));

                setContextEditable(true);
            });

    // qty változás → handler‑UI váltás
    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged),
            this, &AddInputDialog::onQuantityChanged);

    // slider változás → label frissítés
    connect(ui->sliderHandler, &QSlider::valueChanged,
            this, &AddInputDialog::updateSliderLabels);

    // Fókusz fix
    QTimer::singleShot(0, this, [this]() {
        if(ui->editReference->text().isEmpty()){
            ui->editReference->setFocus();
            return;
        }

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

    // connect(ui->editReference, &QLineEdit::textChanged,
    //         this, [this](const QString& ref) {

    //             auto it = _contexts.find(ref.trimmed());
    //             if (it == _contexts.end()) {
    //                 // új tételszám → mezők szabadon szerkeszthetők
    //                 ui->editOwner->setReadOnly(false);
    //                 ui->editDueDate->setReadOnly(false);

    //                 ui->radioButton_alap->setEnabled(true);
    //                 ui->radioButton_rugos->setEnabled(true);
    //                 ui->radioButton_tetoteri->setEnabled(true);
    //                 ui->radioButton_nincs->setEnabled(true);

    //                 ui->radioLeft->setEnabled(true);
    //                 ui->radioRight->setEnabled(true);
    //                 return;
    //             }

    //             const RequestContext& ctx = it.value();

    //             ui->editOwner->setText(ctx.ownerName);
    //             ui->editDueDate->setDate(ctx.dueDate);

    //             switch (ctx.subtype) {
    //             case Subtype::Alap:     ui->radioButton_alap->setChecked(true); break;
    //             case Subtype::Rugos:    ui->radioButton_rugos->setChecked(true); break;
    //             case Subtype::Tetoteri: ui->radioButton_tetoteri->setChecked(true); break;
    //             default:                ui->radioButton_nincs->setChecked(true); break;
    //             }

    //             if (ctx.side == HandlerSide::Left)
    //                 ui->radioLeft->setChecked(true);
    //             else if (ctx.side == HandlerSide::Right)
    //                 ui->radioRight->setChecked(true);

    //             if (!ctx.defaultMaterialId.isNull()) {
    //                 int idx = ui->comboMaterial->findData(ctx.defaultMaterialId);
    //                 if (idx >= 0)
    //                     ui->comboMaterial->setCurrentIndex(idx);
    //             }

    //             // kontextusos tételszám → megrendelői mezők readonly
    //             ui->editOwner->setReadOnly(true);
    //             ui->editDueDate->setReadOnly(true);

    //             ui->radioButton_alap->setEnabled(false);
    //             ui->radioButton_rugos->setEnabled(false);
    //             ui->radioButton_tetoteri->setEnabled(false);
    //             ui->radioButton_nincs->setEnabled(false);

    //             ui->radioLeft->setEnabled(false);
    //             ui->radioRight->setEnabled(false);
    //         });

}


AddInputDialog::~AddInputDialog()
{
    saveOwnerCache();
    saveContextMap();
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

        req.color = ui->edit_Color->text().trimmed();

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

    const QString ref = ui->editReference->text().trimmed();
    s_lastExternalRef = ref;
    s_lastRepeat = ui->chk_Repeat->isChecked();

    RequestContext ctx;
    ctx.ownerName = ui->editOwner->text().trimmed();
    ctx.dueDate   = ui->editDueDate->date();
    ctx.subtype   = parseSubtypeFromRadioButtons();
    ctx.side      = ui->radioLeft->isChecked() ? HandlerSide::Left
                                          : HandlerSide::Right;
    ctx.defaultMaterialId = ui->comboMaterial->currentData().toUuid();

    ctx.color = ui->edit_Color->text().trimmed();

    _contexts[ref] = ctx;

    s_ownerCache.insert(ui->editOwner->text().trimmed());

    QDialog::accept();
}


// void AddInputDialog::setModel(const Cutting::Plan::Request& request) {
//     current_requestId = request.requestId;

//     int index = ui->comboMaterial->findData(request.materialId);
//     if (index >= 0)
//         ui->comboMaterial->setCurrentIndex(index);

//     ui->editLength->setText(QString::number(request.requiredLength));
//     ui->spinQuantity->setValue(request.quantity);
//     ui->editOwner->setText(request.ownerName);
//     ui->editReference->setText(request.externalReference);

//     // handler UI
//     onQuantityChanged(request.quantity); // beállítja a módot + slider range-et

//     // ⭐ J/B visszatöltés
//     onQuantityChanged(request.quantity);

//     if (request.quantity == 1) {
//         ui->radioLeft->setChecked(request.leftCount == 1);
//         ui->radioRight->setChecked(request.rightCount == 1);
//     } else {
//         ui->sliderHandler->setValue(request.leftCount);
//         updateSliderLabels();
//     }

//     // Altípus
//     switch (request.subtype) {
//     case Subtype::Alap:     ui->radioButton_alap->setChecked(true); break;
//     case Subtype::Rugos:    ui->radioButton_rugos->setChecked(true); break;
//     case Subtype::Tetoteri: ui->radioButton_tetoteri->setChecked(true); break;
//     default:                ui->radioButton_nincs->setChecked(true); break;
//     }

//     ui->editDueDate->setDate(request.dueDate);

// }

void AddInputDialog::setModel(const Cutting::Plan::Request& req)
{
    current_requestId = req.requestId;
    applyRequestToWidgets(req);
    setContextEditable(false); // Update módban nem szerkeszthető
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

void AddInputDialog::applyContextToWidgets(const RequestContext& ctx)
{
    zEvent("applyContextToWidgets dueDate:"
           +ctx.dueDate.toString("yyyy-MM-dd"));

    ui->editOwner->setText(ctx.ownerName);
    ui->editDueDate->setDate(ctx.dueDate);

    applySubtype(ctx.subtype);
    applySide(ctx.side);

    if (!ctx.defaultMaterialId.isNull()) {
        int idx = ui->comboMaterial->findData(ctx.defaultMaterialId);
        if (idx >= 0)
            ui->comboMaterial->setCurrentIndex(idx);
    }

        ui->edit_Color->setText(ctx.color);   // ÚJ
}

void AddInputDialog::applyRequestToWidgets(const Cutting::Plan::Request& req)
{
    zEvent("applyRequestToWidgets dueDate:"
           +req.dueDate.toString("yyyy-MM-dd"));

    ui->editOwner->setText(req.ownerName);
    ui->editReference->setText(req.externalReference);
    ui->editDueDate->setDate(req.dueDate);
    ui->edit_Color->setText(req.color);

    // subtype
    applySubtype(req.subtype);
    applySide(req.leftCount > 0 ? HandlerSide::Left : HandlerSide::Right);

    // material
    int idx = ui->comboMaterial->findData(req.materialId);
    if (idx >= 0)
        ui->comboMaterial->setCurrentIndex(idx);

    // length + quantity
    ui->editLength->setText(QString::number(req.requiredLength));
    ui->spinQuantity->setValue(req.quantity);

    onQuantityChanged(req.quantity);
    if (req.quantity == 1)
        ui->radioLeft->setChecked(req.leftCount == 1);
    else
        ui->sliderHandler->setValue(req.leftCount);
}

void AddInputDialog::applySubtype(Subtype t)
{
    switch (t) {
    case Subtype::Alap:     ui->radioButton_alap->setChecked(true); break;
    case Subtype::Rugos:    ui->radioButton_rugos->setChecked(true); break;
    case Subtype::Tetoteri: ui->radioButton_tetoteri->setChecked(true); break;
    default:                ui->radioButton_nincs->setChecked(true); break;
    }
}

void AddInputDialog::applySide(HandlerSide side)
{
    if (side == HandlerSide::Left)
        ui->radioLeft->setChecked(true);
    else if (side == HandlerSide::Right)
        ui->radioRight->setChecked(true);
}


void AddInputDialog::setContextEditable(bool editable)
{
    //ui->editOwner->setReadOnly(!editable);
    //ui->editDueDate->setReadOnly(!editable);
    // ⭐ Dátum popup tiltása readonly módban
    //ui->editDueDate->setCalendarPopup(editable);

    ui->editOwner->setEnabled(editable);
    ui->editDueDate->setEnabled(editable);

    ui->radioButton_alap->setEnabled(editable);
    ui->radioButton_rugos->setEnabled(editable);
    ui->radioButton_tetoteri->setEnabled(editable);
    ui->radioButton_nincs->setEnabled(editable);

    ui->radioLeft->setEnabled(editable);
    ui->radioRight->setEnabled(editable);

    ui->edit_Color->setEnabled(editable);

}

void AddInputDialog::loadOwnerCache()
{
    QFile f(OWNER_CACHE_FN);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (!line.isEmpty())
            s_ownerCache.insert(line);
    }
}

void AddInputDialog::saveOwnerCache()
{
    QFile f(OWNER_CACHE_FN);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    for (const QString& name : s_ownerCache)
        f.write((name + "\n").toUtf8());
}

void AddInputDialog::on_btn_Reset_clicked()
{
    // Kritikus mezők reset
    ui->editOwner->clear();
    ui->editDueDate->setDate(QDate::currentDate().addDays(1));
    ui->radioButton_nincs->setChecked(true);
    ui->radioLeft->setChecked(true);
    ui->comboMaterial->setCurrentIndex(0);
    ui->editLength->clear();
    ui->spinQuantity->setValue(1);
    ui->edit_Color->clear();

    // Új adat bevitel engedélyezése
    setContextEditable(true);
}

void AddInputDialog::loadContextMap()
{
    QFile f(CONTEXT_CACHE_FN);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty()) continue;

        auto parts = line.split(';');

        // régi formátum: 6 mező
        // új formátum: 7 mező
        if (parts.size() < 6)
            continue;

        RequestContext ctx;
        ctx.ownerName = parts[1];
        ctx.dueDate = QDate::fromString(parts[2], "yyyy-MM-dd");
        ctx.subtype = SubtypeUtils::parse(parts[3]);
        ctx.side = HandlerSideUtils::parse(parts[4]);
        //ctx.defaultMaterialId = QUuid(parts[5]);


        auto matBarcode = parts[5];
        auto* mat = MaterialRegistry::instance().findByBarcode(matBarcode);
        ctx.defaultMaterialId = mat?mat->id:QUuid();

        // ÚJ: szín mező, ha van
        if (parts.size() >= 7)
            ctx.color = parts[6];
        else
            ctx.color = "";   // régi sor → üres szín

        _contexts[parts[0]] = ctx;
    }
}



void AddInputDialog::saveContextMap()
{
    QFile f(CONTEXT_CACHE_FN);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    for (auto it = _contexts.begin(); it != _contexts.end(); ++it) {
        const auto& ctx = it.value();

        auto *mat = MaterialRegistry::instance().findById(ctx.defaultMaterialId);
        QString matBarcode = mat?mat->barcode:"";

        QString line = QString("%1;%2;%3;%4;%5;%6;%7\n")
                           .arg(it.key())
                           .arg(ctx.ownerName)
                           .arg(ctx.dueDate.toString("yyyy-MM-dd"))
                           .arg(SubtypeUtils::toString_CSV(ctx.subtype))
                           .arg(HandlerSideUtils::toDisplayText(ctx.side))
                           .arg(matBarcode)
                           .arg(ctx.color);

        f.write(line.toUtf8());
    }
}





