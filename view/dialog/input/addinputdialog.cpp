#include "addinputdialog.h"

#include "../../../model/cutting/plan/request.h"
#include "model/cutting/plan/audit/naphalo_profile_postfix.h"
#include "ui_addinputdialog.h"
#include "materials/registry/material_registry.h"
#include "view/common/layouts/qflowlayout.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"

#include <QCompleter>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <common/eventlogger.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

QString AddInputDialog::s_lastExternalRef;
//QMap<QString, AddInputDialog::RequestContext> AddInputDialog::_contexts;
QSet<QString> AddInputDialog::s_ownerCache;

bool AddInputDialog::s_lastRepeat = false;

AddInputDialog::AddInputDialog(QWidget *parent,
                               DialogMode mode,
                               const Cutting::Plan::Request* initial,
                               SeriesMatrixView* matrix)
    : QDialog(parent)
    , ui(new Ui::AddInputDialog)
    , _matrix(matrix)
    , current_requestId(QUuid::createUuid())

{
    // if (_contexts.isEmpty()) {
    //     loadContextMap();
    // }
    _editMode = EditMode::None;

    if (s_ownerCache.isEmpty()) {
        loadOwnerCache();
    }

    _mode = mode;
    _shiftEnterAccepted = false;

    ui->setupUi(this);

    ui->editReference->installEventFilter(this);

    // NINCS stackReferenceMode használat
    lblReferenceBig    = ui->lblReferenceBig;
    btnEditReference   = ui->btnEditReference;
    btnNextRef         = ui->btnNextRef;
    btnNextMaterial    = ui->btnNextMaterial;
    btnFirstRef = ui->btnFirstRef;

    // Kezdő állapot: nincs tételszám → edit mode
    if (mode != DialogMode::Update) {
        enterReferenceEditMode();
        lockAllFieldsUntilReference();
    }

    connect(ui->btnEditReference, &QToolButton::clicked,
            this, &AddInputDialog::on_btnEditReference_clicked);

    // ⭐ ProductType layout
    auto* typeLayout = new QFlowLayout(ui->groupBox_productType);
    ui->groupBox_productType->setLayout(typeLayout);
    // ⭐ ProductType rádiógombok dinamikus generálása
    //auto* typeLayout = ui->groupBox_productType->findChild<QVBoxLayout*>("verticalLayout");
    for (const auto& type : ProductTypeRegistry::instance().readAll()) {
        auto* rb = new QRadioButton(type.name, this);
        rb->setProperty("typeId", type.id);
        typeLayout->addWidget(rb);
        connect(rb, &QRadioButton::toggled, this, &AddInputDialog::onProductTypeChanged);
    }

    auto* subtypeStack = ui->stackedWidget_stackSubtype;

    // ⭐ QStackedWidget old page-ek eltávolítása
    while (subtypeStack->count() > 0) {
        QWidget* w = subtypeStack->widget(0);
        subtypeStack->removeWidget(w);
        w->deleteLater();
    }

    // ⭐ ProductSubtype panelek dinamikus generálása
    for (const auto& type : ProductTypeRegistry::instance().readAll()) {
        auto* page = new QWidget(this);
        page->setProperty("typeId", type.id);
        //auto* lay = new QVBoxLayout(page);
        auto* lay = new QFlowLayout(page);

        for (const auto& st : ProductSubtypeRegistry::instance().findByTypeId(type.id)) {
            auto* rb = new QRadioButton(st.name, page);
            rb->setProperty("subtypeId", st.id);
            lay->addWidget(rb);
        }

        subtypeStack->addWidget(page);
    }

    ui->editLength->installEventFilter(this);

    auto completer = new QCompleter(s_ownerCache.values(), this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->editOwner->setCompleter(completer);

    populateMaterialCombo();

    populateSurfaceCombo();
    connect(ui->edit_Color, &QLineEdit::textChanged, this, &AddInputDialog::updateColorPreview);
    connect(ui->comboMaterial, qOverload<int>(&QComboBox::currentIndexChanged), this, &AddInputDialog::updateColorPreview);
    connect(ui->editLength, &QLineEdit::textChanged, this, &AddInputDialog::updateColorPreview);
    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged), this, &AddInputDialog::updateColorPreview);


    // ⭐ Tiszta tételszám ajánlás
    //_nextSuggestedRef = computeNextReference();

    connect(ui->editReference, &QLineEdit::editingFinished,this, [this]() {
        QString ref = ui->editReference->text().trimmed();
        if (ref.isEmpty()) {
            enterReferenceEditMode();      // mindent vissza beíró módba
            return;
        }

        _editMode = EditMode::None;
        loadReference(ref);
    });

    connect(btnFirstRef, &QPushButton::clicked, this, [this]() {
        QString first = CuttingPlanRequestRegistry::instance().getFirstReference();
        if (first.isEmpty())
            return;

        // ⭐ 2) UI frissítés
        ui->editReference->setText(first);
        _editMode = EditMode::None;
        loadReference(first);
    });

    connect(btnNextRef, &QPushButton::clicked, this, [this]() {
        QString next = computeNextItemNumber();
        if (next.isEmpty())
            return;

        ui->editReference->setText(next);
        _editMode = EditMode::None;
        loadReference(next);
    });

    connect(btnNextMaterial, &QPushButton::clicked, this, [this]() {

        QString ref = ui->editReference->text().trimmed();
        if (ref.isEmpty())
            return;

        QUuid nextMat = computeNextMaterialForCurrentRef();
        if (nextMat.isNull())
            return;

        int idx = ui->comboMaterial->findData(nextMat);
        if (idx >= 0){
            _editMode = EditMode::ItemEdit;
            applyReferenceState(ReferenceState::FullRequest);
            ui->comboMaterial->setCurrentIndex(idx);
            ui->editLength->clear();
            ui->editLength->setFocus();
        }
    });

    // qty változás → handler‑UI váltás
    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged),
            this, &AddInputDialog::onQuantityChanged);

    // slider változás → label frissítés
    connect(ui->sliderHandler, &QSlider::valueChanged,
            this, &AddInputDialog::updateSliderLabels);


    // // ⭐ MODE-AWARE INITIALIZATION
    // if (mode == DialogMode::Update && initial) {

    //     // késleltetett inicializálás, amikor a UI már stabil
    //     QTimer::singleShot(0, this, [this, initial]() {

    //         current_requestId = initial->requestId;

    //         applyRequestToWidgets(*initial);

    //         setHeadEditable(true);
    //         ui->chk_Repeat->hide();

    //         ui->editReference->setText(initial->externalReference);
    //         loadReference(initial->externalReference);

    //         applyInitialFocus();
    //     });

    //     return;
    // }

    connect(ui->btnEditHead, &QPushButton::clicked, this, [this]() {
        if(_editMode != EditMode::HeadEdit){
            _editMode = EditMode::HeadEdit;
        } else{
            _editMode = EditMode::None;
        }
        loadReference(ui->editReference->text().trimmed());
    });

    connect(ui->btnEditItem, &QPushButton::clicked, this, [this]() {
        if(_editMode != EditMode::ItemEdit){
            _editMode = EditMode::ItemEdit;
        } else{
            _editMode = EditMode::None;
        }

        loadReference(ui->editReference->text().trimmed());
    });

    connect(ui->chkUnknownSide, &QCheckBox::toggled, this, [this](bool checked){
        if (checked) {
            // UNKNOWN állapot aktiválása
            ui->radioLeft->setChecked(false);
            ui->radioRight->setChecked(false);
            ui->sliderHandler->setValue(0);
        }
        // ha unchecked → nem csinálunk semmit
        // mert a rádiók/slider majd maguk állítják be a helyes oldalt
    });

    connect(ui->radioLeft, &QRadioButton::toggled, this, [this](bool checked){
        if (checked)
            ui->chkUnknownSide->setChecked(false);
    });

    connect(ui->radioRight, &QRadioButton::toggled, this, [this](bool checked){
        if (checked)
            ui->chkUnknownSide->setChecked(false);
    });

    connect(ui->sliderHandler, &QSlider::valueChanged, this, [this](int){
        ui->chkUnknownSide->setChecked(false);
    });


    // ⭐ Induló inicializálás
    // QTimer::singleShot(0, this, [this]() {

    //     initializeDialog();
    //     ui->chk_Repeat->setChecked(s_lastRepeat);   // ⭐ repeat visszatöltése
    //     //_originalReference = ui->editReference->text().trimmed();   // ⭐ eredeti érték mentése


    // });

    QTimer::singleShot(0, this, [this, mode, initial]() {

        if (mode == DialogMode::Update && initial) {

            current_requestId = initial->requestId;

            applyRequestToWidgets(*initial);

            setHeadEditable(true);
            ui->chk_Repeat->hide();

            ui->editReference->setText(initial->externalReference);
            loadReference(initial->externalReference);

            applyInitialFocus();
        }
        else {
            initializeDialog();
            ui->chk_Repeat->setChecked(s_lastRepeat);
        }
        _suppressPreview = false;
        updateColorPreview();

    });

}

AddInputDialog::~AddInputDialog()
{
    saveOwnerCache();
    //saveContextMap();
    delete ui;
}


void AddInputDialog::initializeDialog()
{
    QString ref = ui->editReference->text().trimmed();

    // Repeat BOM workflow → ugyanazon tételszámon folytatunk
    if (ref.isEmpty() && s_lastRepeat && !s_lastExternalRef.isEmpty()) {
        ref = s_lastExternalRef;
        ui->editReference->setText(ref);
    }

    if (ref.isEmpty()) {
        enterReferenceEditMode();
        return;
    }

    loadReference(ref);

    emit seriesContextChanged(ui->editOwner->text().trimmed(), ref);
}




void AddInputDialog::resetUiForExistingReference()
{
    // jelenleg üres — később bővíthető
    setHeadEditable(true);
}

void AddInputDialog::applyInitialFocus()
{
    if (ui->editReference->text().isEmpty()) {
        ui->editReference->setFocus();
        return;
    }

    if (ui->editOwner->text().isEmpty()) {
        ui->editOwner->setFocus();
        return;
    }

    if (ui->editLength->text().isEmpty()) {
        ui->editLength->setFocus();
        return;
    }

    ui->editLength->setFocus();
    ui->editLength->selectAll();
}

QString AddInputDialog::computeNextReference()
{
    if (s_lastExternalRef.isEmpty())
        return QString();

    bool ok = false;
    int num = s_lastExternalRef.toInt(&ok);
    if (!ok)
        return QString();

    return QString::number(num + 1);
}

// --- UI reset metódusok új nevekkel ---
void AddInputDialog::resetUiForNewReference()
{
    ui->editOwner->clear();
    ui->editDueDate->setDate(QDate::currentDate().addDays(1));
    ui->edit_Color->clear();
    ui->comboBox_Surface->setCurrentIndex(-1);
    ui->editLength->clear();
    ui->spinQuantity->setValue(1);

    ui->radioLeft->setAutoExclusive(false);
    ui->radioRight->setAutoExclusive(false);
    ui->radioLeft->setChecked(false);
    ui->radioRight->setChecked(false);
    ui->radioLeft->setAutoExclusive(true);
    ui->radioRight->setAutoExclusive(true);

    ui->sliderHandler->setValue(0);
    setHeadEditable(true);
}

void AddInputDialog::resetUiForNextReference()
{
    ui->editLength->clear();

    ui->radioLeft->setAutoExclusive(false);
    ui->radioRight->setAutoExclusive(false);
    ui->radioLeft->setChecked(false);
    ui->radioRight->setChecked(false);
    ui->radioLeft->setAutoExclusive(true);
    ui->radioRight->setAutoExclusive(true);

    ui->sliderHandler->setValue(0);
    setHeadEditable(true);
}

void AddInputDialog::populateMaterialCombo() {
    const auto& registry = MaterialRegistry::instance().readAll();

    ui->comboMaterial->clear();
    for (const auto& m : registry) {
        if (m.cuttingMode != CuttingMode::Length)
            continue;

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

    if (!ui->chkUnknownSide->isChecked()) {
        // ⭐ J/B darabszámok
        if (totalPieces == 1) {
            req.leftCount  = ui->radioLeft->isChecked() ? 1 : 0;
            req.rightCount = ui->radioRight->isChecked() ? 1 : 0;
        }
         else {
            int right = ui->sliderHandler->value();
            int left  = totalPieces - right;

            req.leftCount  = left;
            req.rightCount = right;

        }
    } else{
        req.leftCount = 0;
        req.rightCount = 0;
    }

    // ⭐ ProductType (getter)
    req.productTypeId = selectedProductTypeId();

    // ⭐ ProductSubtype (getter — NEM mezőből!)
    req.productSubtypeId = selectedProductSubtypeId();

    req.dueDate = ui->editDueDate->date();

    QString colorName = ui->edit_Color->text().trimmed();
    //req.color = colorName;
    req.requiredColor = NamedColor::fromUserInput(colorName);

    QVariant surfaceCode = ui->comboBox_Surface->currentData();
    if (surfaceCode.isValid())
        req.surface = SurfaceTypeUtils::fromCode(surfaceCode.toString());

    return req;
}



bool AddInputDialog::validateInputs() {

    Cutting::Plan::Request req = getModel();
    QStringList errors;

    // 1) J/B darabszám
    const int totalPieces = req.quantity;
    const int l = req.leftCount;
    const int r = req.rightCount;

    // Ha nincs megadva → engedjük
    if (!(l == 0 && r == 0)) {
        // Ha meg van adva → validálni kell
        if (l + r != totalPieces) {
            errors << "A balos és jobbos darabszám összege nem egyezik meg a teljes darabszámmal.";
        }
    }

    // 2) Request saját hibái
    errors << req.invalidReasons();

    // 3) Hossz
    if (req.requiredLength < 100)
        errors << "A vágási hossz nem lehet 100 mm alatt.";
    if (req.requiredLength < 200)
        errors << "200 mm alatti darabot nem vágunk.";

    // 4) Dátum
    if (!req.dueDate.isValid())
        errors << "A határidő érvénytelen.";

    // 5) Type/Subtype
    if (req.productTypeId.isNull())
        errors << "Nincs kiválasztva terméktípus.";
    if (req.productSubtypeId.isNull())
        errors << "Nincs kiválasztva altípus.";

    // 6) Ha van hiba → egyetlen ablak
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

    Cutting::Plan::Request req = getModel();   // ⭐ egyetlen forrás


    // ⭐ PATCH #3 — jelzés a SeriesMatrixView felé
    emit seriesContextChanged(req.ownerName, req.externalReference);

    const QString ref = req.externalReference;
    s_lastExternalRef = ref;
    s_lastRepeat = ui->chk_Repeat->isChecked();

    s_ownerCache.insert(req.ownerName);

    updateHeadFieldsInRegistry(req.externalReference);

    QDialog::accept();
}

bool AddInputDialog::shouldRepeat()
{
    return ui->chk_Repeat->isChecked();
}

void AddInputDialog::onQuantityChanged(int totalPieces)
{
    if (totalPieces <= 1) {
        ui->stackHandlerSide->setCurrentIndex(0);   // radio mód
        //ui->radioLeft->setChecked(true);            // default: balos
        ui->radioLeft->setAutoExclusive(false);
        ui->radioRight->setAutoExclusive(false);

        ui->radioLeft->setChecked(false);
        ui->radioRight->setChecked(false);

        ui->radioLeft->setAutoExclusive(true);
        ui->radioRight->setAutoExclusive(true);
        ui->sliderHandler->setValue(0);
    } else {
        ui->stackHandlerSide->setCurrentIndex(1);   // slider mód
        ui->sliderHandler->setMinimum(0);
        ui->sliderHandler->setMaximum(totalPieces);
        //ui->sliderHandler->setValue(totalPieces);   // default: minden bal
    }
    updateSliderLabels();
}

void AddInputDialog::updateSliderLabels()
{
    int totalPieces = ui->spinQuantity->value();
    int right        = ui->sliderHandler->value();
    int left       = totalPieces - right;

    ui->labelLeftValue->setText(QString::number(left));
    ui->labelRightValue->setText(QString::number(right));
}

void AddInputDialog::keyPressEvent(QKeyEvent *e)
{
    // Enter → OK, de csak ha nem QLineEdit-ben vagyunk
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        QWidget *w = focusWidget();

        // ⭐ ENTER a tételszám mezőben → csak módváltás, NEM accept
        if (w == ui->editReference)
        {
            QString ref = ui->editReference->text().trimmed();
            if (!ref.isEmpty())
            {
                loadReference(ref);
            }
            return;   // ❗ Kritikus: NEM acceptálunk
        }

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
    // ⭐ Tételszám mező speciális ENTER-kezelése
    if (obj == ui->editReference && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(event);

        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {

            QString ref = ui->editReference->text().trimmed();
            if (!ref.isEmpty()) {
                loadReference(ref);
            }

            // ⭐ Kritikus: elnyeljük az eseményt → NEM megy tovább a dialoghoz
            return true;
        }
    }

    // meglévő length-selectAll logika
    if (obj == ui->editLength && event->type() == QEvent::FocusIn) {
        ui->editLength->selectAll();
    }
    return QDialog::eventFilter(obj, event);
}


void AddInputDialog::on_btn_MaterialSearch_clicked()
{

    // a színt is át lehetne adni

    // 1) Jelenlegi típus és altípus lekérése
    QUuid typeId = selectedProductTypeId();
    QUuid subtypeId = selectedProductSubtypeId();

    QString typeCode = "Mind";
    QString subtypeCode = "Mind";

    if (!typeId.isNull()) {
        auto* t = ProductTypeRegistry::instance().findById(typeId);
        if (t) typeCode = t->code;
    }

    if (!subtypeId.isNull()) {
        auto* st = ProductSubtypeRegistry::instance().findById(subtypeId);
        if (st) subtypeCode = st->code;
    }

    QString currentColor = ui->edit_Color->text().trimmed();
    if (currentColor.isEmpty()){
        currentColor = "Nincs";
    }else{
        currentColor = NamedColor::normalizeRalExtended(currentColor);
    }

    // 2) MaterialSearchDialog előre beállított szűrővel
    MaterialSearchDialog dlg(
        this,
        currentColor,   // ⭐ SZÍN ÁTADÁSA,        // initialColor
        typeCode,       // initialType
        subtypeCode,    // initialSubtype
        ""              // initialSearch
        );

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


void AddInputDialog::applyRequestToWidgets(const Cutting::Plan::Request& req)
{
    bool _suppressPreview = true;
    applyFields_Head(req);
    applyFields_Item(req);
}

void AddInputDialog::applySide(HandlerSide side)
{
    if(side == HandlerSide::None){
        ui->radioLeft->setChecked(false);
        ui->radioRight->setChecked(false);
        ui->chkUnknownSide->setChecked(true);
    } else{
        ui->chkUnknownSide->setChecked(false);
        if (side == HandlerSide::Left)
            ui->radioLeft->setChecked(true);
        else if (side == HandlerSide::Right)
            ui->radioRight->setChecked(true);
    }
}

void AddInputDialog::applySide_Slider(int l, int r)
{
    if(l == 0 && r == 0){
        ui->chkUnknownSide->setChecked(true);
        ui->sliderHandler->setValue(0);
    } else{
        ui->chkUnknownSide->setChecked(false);
        ui->sliderHandler->setValue(r);
    }
}

void AddInputDialog::setHeadEditable(bool editable)
{
    setReferenceEditable(editable);
    setOwnerEditable(editable);
    setDateEditable(editable);

    setProductTypeEditable(editable);
    setProductSubtypeEditable(editable);

    setSideEditable(editable);
    setColorEditable(editable);
    setSurfaceEditable(editable);
    setQuantityEditable(editable);
}

void AddInputDialog::setItemEditable(bool editable){
    setLengthEditable(editable);
    setMaterialEditable(editable);
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

    // ⭐ Referencia mező törlése
    ui->editReference->clear();
    //_originalReference.clear();

    // ⭐ Reset modul használata
    resetUiForNewReference();

    // ⭐ Szerkeszthető mezők engedélyezése
    setHeadEditable(true);

    // ⭐ Fókusz beállítása
    applyInitialFocus();

    s_lastRepeat = false;
}




void AddInputDialog::setOwnerEditable(bool editable)
{
    ui->editOwner->setEnabled(editable);
}
void AddInputDialog::setDateEditable(bool editable)
{
    ui->editDueDate->setEnabled(editable);
}

void AddInputDialog::setSideEditable(bool editable)
{
    ui->stackHandlerSide->setEnabled(editable);
    ui->radioLeft->setEnabled(editable);
    ui->radioRight->setEnabled(editable);
}
void AddInputDialog::setColorEditable(bool editable)
{
    ui->edit_Color->setEnabled(editable);
}
void AddInputDialog::setSurfaceEditable(bool editable)
{
    ui->comboBox_Surface->setEnabled(editable);
}

void AddInputDialog::setQuantityEditable(bool editable)
{
    ui->spinQuantity->setEnabled(editable);
}

void AddInputDialog::setLengthEditable(bool editable)
{
    ui->editLength->setEnabled(editable);
}

void AddInputDialog::setMaterialEditable(bool editable)
{
    ui->comboMaterial->setEnabled(editable);
    ui->btn_MaterialSearch->setEnabled(editable);
}

void AddInputDialog::applyOwnerFromRequest(const Cutting::Plan::Request& req)
{
    ui->editOwner->setText(req.ownerName);
}

void AddInputDialog::applyReferenceFromRequest(const Cutting::Plan::Request& req)
{
    ui->editReference->setText(req.externalReference);
}
void AddInputDialog::applyDateFromRequest(const Cutting::Plan::Request& req)
{
    ui->editDueDate->setDate(req.dueDate);
}
void AddInputDialog::applyColorFromRequest(const Cutting::Plan::Request& req)
{
    ui->edit_Color->setText(req.requiredColor.code());
}

void AddInputDialog::applySideFromRequest(const Cutting::Plan::Request& req)
{
    if(req.leftCount == 0 && req.rightCount == 0){
        if(req.quantity>1){
            applySide_Slider(0,0);
            ui->sliderHandler->setMaximum(1);
        } else{
            applySide(HandlerSide::None);
        }
    } else{
        if(req.quantity>1){
            ui->sliderHandler->setMaximum(req.quantity);
            applySide_Slider(req.leftCount, req.rightCount);
        } else{
            applySide(req.leftCount > 0 ? HandlerSide::Left : HandlerSide::Right);
        }
    }
}

void AddInputDialog::applyMaterialFromRequest(const Cutting::Plan::Request& req)
{
    int idx = ui->comboMaterial->findData(req.materialId);
    if (idx >= 0)
        ui->comboMaterial->setCurrentIndex(idx);
}

void AddInputDialog::applyLengthFromRequest(const Cutting::Plan::Request& req)
{
    ui->editLength->setText(QString::number(req.requiredLength));
}

void AddInputDialog::applyQuantityFromRequest(const Cutting::Plan::Request& req)
{
    ui->spinQuantity->setValue(req.quantity);

    // onQuantityChanged(req.quantity);

    // if (req.quantity == 1)
    //     ui->radioLeft->setChecked(req.leftCount == 1);
    // else
    //     ui->sliderHandler->setValue(req.leftCount);
}

void AddInputDialog::setReferenceEditable(bool editable)
{
    ui->editReference->setEnabled(editable);
}

void AddInputDialog::onProductTypeChanged(bool checked)
{
    if (!checked)
        return;

    QUuid typeId = sender()->property("typeId").toUuid();
    auto* stack = ui->stackedWidget_stackSubtype;

    for (int i = 0; i < stack->count(); ++i) {
        QWidget* page = stack->widget(i);
        if (page->property("typeId").toUuid() == typeId) {
            stack->setCurrentWidget(page);
            break;
        }
    }
}


QUuid AddInputDialog::selectedProductTypeId() const
{
    for (auto* rb : ui->groupBox_productType->findChildren<QRadioButton*>()) {
        if (rb->isChecked())
            return rb->property("typeId").toUuid();
    }
    return QUuid(); // nincs kiválasztva
}

QUuid AddInputDialog::selectedProductSubtypeId() const
{
    auto* stack = ui->stackedWidget_stackSubtype;
    QWidget* page = stack->currentWidget();
    if (!page)
        return QUuid();

    for (auto* rb : page->findChildren<QRadioButton*>()) {
        if (rb->isChecked())
            return rb->property("subtypeId").toUuid();
    }

    return QUuid();
}

void AddInputDialog::setSelectedProductTypeId(const QUuid& typeId)
{
    for (auto* rb : ui->groupBox_productType->findChildren<QRadioButton*>()) {
        if (rb->property("typeId").toUuid() == typeId) {
            rb->setChecked(true);
            return;
        }
    }
}

void AddInputDialog::setSelectedProductSubtypeId(const QUuid& subtypeId)
{
    auto* stack = ui->stackedWidget_stackSubtype;

    for (int i = 0; i < stack->count(); ++i) {
        QWidget* page = stack->widget(i);

        // subtype page must match the selected type
        for (auto* rb : page->findChildren<QRadioButton*>()) {
            if (rb->property("subtypeId").toUuid() == subtypeId) {
                stack->setCurrentWidget(page);
                rb->setChecked(true);
                return;
            }
        }
    }
}

void AddInputDialog::applyProductTypeFromRequest(const Cutting::Plan::Request& req)
{
    setSelectedProductTypeId(req.productTypeId);
}
void AddInputDialog::applyProductSubtypeFromRequest(const Cutting::Plan::Request& req)
{
    setSelectedProductSubtypeId(req.productSubtypeId);
}
void AddInputDialog::setProductTypeEditable(bool editable)
{
    ui->groupBox_productType->setEnabled(editable);
    for (auto* rb : ui->groupBox_productType->findChildren<QRadioButton*>())
        rb->setEnabled(editable);
}
void AddInputDialog::setProductSubtypeEditable(bool editable)
{
    auto* stack = ui->stackedWidget_stackSubtype;
    stack->setEnabled(editable);
    for (int i = 0; i < stack->count(); ++i) {
        QWidget* page = stack->widget(i);
        for (auto* rb : page->findChildren<QRadioButton*>())
            rb->setEnabled(editable);
    }
}

void AddInputDialog::populateSurfaceCombo() {
    ui->comboBox_Surface->clear();
    ui->comboBox_Surface->addItem("Smooth", "SM");
    ui->comboBox_Surface->addItem("Fine Structure", "FS");
    ui->comboBox_Surface->addItem("Coarse Structure", "CS");
    ui->comboBox_Surface->addItem("Matt", "MT");
    ui->comboBox_Surface->addItem("Glossy", "GL");
    ui->comboBox_Surface->addItem("Satin", "ST");
}


void AddInputDialog::applySurfaceFromRequest(const Cutting::Plan::Request& req)
{
    QString code = SurfaceTypeUtils::toCode(req.surface);
    int idx = ui->comboBox_Surface->findData(code);
    if (idx >= 0)
        ui->comboBox_Surface->setCurrentIndex(idx);
}

void AddInputDialog::updateColorPreview()
{

    if (_suppressPreview)
        return;

    // 1) Layout közvetlen elérése
    auto* lay = ui->layout_colorPreview;
    if (!lay)
        return;

    // 2) Layout kiürítése
    while (QLayoutItem* item = lay->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }

    // 3) Szín értelmezése
    QString raw = ui->edit_Color->text().trimmed();
    NamedColor nc = NamedColor::fromUserInput(raw);

    QUuid matId = selectedMaterialId();
    const MaterialMaster* mat = MaterialRegistry::instance().findById(matId);

    bool paintingNeeded = false;
    if (mat && nc.isValid() && mat->paintingMode != PaintingMode::None) {
        paintingNeeded = (nc.code() != mat->color.code());
    }

    int len   = ui->editLength->text().toInt();
    int qty   = ui->spinQuantity->value();
    int total = (paintingNeeded ? len * qty : 0);
    QString postfix = (mat ? profilePostfixFor(mat->barcode) : QString());

    // 4) Szín kocka
    QLabel* box = new QLabel();
    box->setFixedSize(16, 16);
    if (nc.isValid())
        box->setStyleSheet(
            QString("background-color:%1; border:1px solid #444;")
                .arg(nc.color().name()));
    lay->addWidget(box);

    // 5) Kód + név
    QLabel* codeLbl = new QLabel(nc.isValid() ? nc.code() : QString("Ismeretlen"), this);
    lay->addWidget(codeLbl);

    QLabel* nameLbl = new QLabel(nc.isValid() ? nc.name() : QString(), this);
    lay->addWidget(nameLbl);

    // 6) Festés info
    if (paintingNeeded && mat && mat->paintingMode != PaintingMode::None) {
        QLabel* icon = new QLabel("🖌️", this);
        lay->addWidget(icon);

        QString text =
            postfix.isEmpty()
                ? QString("%1 mm").arg(total)
                : QString("%1 mm (%2 m × %3)")
                      .arg(total)
                      .arg(total / 1000.0, 0, 'f', 2)
                      .arg(postfix);

        QLabel* lenLbl = new QLabel(text, this);
        lay->addWidget(lenLbl);
    }

    lay->addStretch();
}

// void AddInputDialog::updateSeriesStateAfterAccept(const Cutting::Plan::Request& req)
// {
//     // 1) Ha szerkesztünk → csak cella frissítés
//     if (_contextMode == ContextMode::Update) {
//         _series.filledCells.insert({ req.externalReference, req.materialId });
//         _series.editingMode = true;   // ← VISSZAJÖHET
//         return;
//     }

//     // 2) Sorozat indulása
//     if (!_series.active) {
//         _series.active = true;
//         _series.startRef = req.externalReference;
//         _series.order.clear();
//         _series.order.append(req.externalReference);

//         // BOM anyagok betöltése
//         loadBomMaterials(req);

//         _series.currentMaterialIndex = 0;
//         _series.currentColumnIndex = 0;
//     }

//     // 3) Új tételszám hozzáadása
//     if (!_series.order.contains(req.externalReference)) {
//         _series.order.append(req.externalReference);
//     }

//     // 4) Cellakitöltés
//     _series.filledCells.insert({ req.externalReference, req.materialId });

//     // 5) Oszlop index frissítése
//     int ix = _series.order.indexOf(req.externalReference);
//     if (ix >= 0)
//         _series.currentColumnIndex = ix;
// }



// void AddInputDialog::updateSeriesStateAfterEditingFinished(const QString& ref)
// {
//     if (!_series.active)
//         return;

//     // 1) Ha szerkesztünk → nincs anyagváltás
//     if (_contextMode == ContextMode::Update) {
//         _series.editingMode = true;
//         return;
//     }

//     _series.editingMode = false;

//     // 2) Ha visszatértünk a startRef-hez → anyagváltás
//     if (ref == _series.startRef) {
//         _series.currentMaterialIndex++;
//         if (_series.currentMaterialIndex >= _series.bomMaterials.size())
//             _series.currentMaterialIndex = 0;
//     }

//     // 3) Oszlop index frissítése
//     int ix = _series.order.indexOf(ref);
//     if (ix >= 0)
//         _series.currentColumnIndex = ix;
// }

void AddInputDialog::lockAllFieldsUntilReference()
{
    // ⭐ Csak a tételszám mező aktív
    ui->editReference->setEnabled(true);

    ui->editOwner->setEnabled(false);
    ui->editDueDate->setEnabled(false);
    ui->comboMaterial->setEnabled(false);
    ui->btn_MaterialSearch->setEnabled(false);
    ui->edit_Color->setEnabled(false);
    ui->comboBox_Surface->setEnabled(false);
    ui->editLength->setEnabled(false);
    ui->spinQuantity->setEnabled(false);
    ui->stackHandlerSide->setEnabled(false);
    ui->groupBox_productType->setEnabled(false);
    ui->stackedWidget_stackSubtype->setEnabled(false);

}

// void AddInputDialog::unlockAllFieldsAfterReference()
// {
//     ui->editOwner->setEnabled(true);
//     ui->editDueDate->setEnabled(true);
//     ui->comboMaterial->setEnabled(true);
//     ui->btn_MaterialSearch->setEnabled(true);
//     ui->edit_Color->setEnabled(true);
//     ui->comboBox_Surface->setEnabled(true);
//     ui->editLength->setEnabled(true);
//     ui->spinQuantity->setEnabled(true);
//     ui->stackHandlerSide->setEnabled(true);
//     ui->groupBox_productType->setEnabled(true);
//     ui->stackedWidget_stackSubtype->setEnabled(true);
// }

void AddInputDialog::on_btnEditReference_clicked(bool checked)
{
    Q_UNUSED(checked);
    enterReferenceEditMode();
}

void AddInputDialog::updateSeriesNavigationButtons()
{
    bool enable = true;///*_series.active &&*/ _contextMode != ContextMode::Update;

    btnNextRef->setEnabled(enable);
    btnNextMaterial->setEnabled(enable);
}

// void AddInputDialog::loadBomMaterials(const Cutting::Plan::Request& req)
// {
//     _series.bomMaterials.clear();

//     auto roles = MaterialRoleRegistry::instance()
//                      .findRoles(req.productTypeId, req.productSubtypeId);

//     NamedColor reqColor = req.requiredColor;

//     for (const auto& role : roles) {

//         QString prefix = role.barcodePrefix.trimmed();
//         if (prefix.endsWith("*"))
//             prefix.chop(1);

//         for (const auto& mat : MaterialRegistry::instance().readAll()) {

//             if (!mat.barcode.startsWith(prefix))
//                 continue;

//             if (mat.color.code() != reqColor.code())
//                 continue;

//             _series.bomMaterials.append(mat.id);
//         }
//     }
// }

void AddInputDialog::enterReferenceEditMode()
{
    // Csak beírás, minden más tiltva
    ui->editReference->show();
    ui->editReference->setEnabled(true);
    ui->editReference->setFocus();
    ui->editReference->selectAll();

    lblReferenceBig->hide();
    btnEditReference->hide();
    btnNextRef->hide();
    btnNextMaterial->hide();

    lockAllFieldsUntilReference();
}

void AddInputDialog::loadReference(const QString& ref)
{
    ui->editReference->hide();
    ui->editReference->setEnabled(false);

    lblReferenceBig->setText(ref);
    lblReferenceBig->show();

    btnEditReference->show();
    btnNextRef->show();
    btnNextMaterial->show();

    //unlockAllFieldsAfterReference();

    // fejmezők visszatöltése registryből
    //loadHeadFieldsFromRegistry(ref);

    auto* last = CuttingPlanRequestRegistry::instance().getLastRequest(ref);
    auto state = getReferenceState(ref);
    applyReferenceState(state);

    if(last){
        applyRequestToWidgets(*last);
    }else {
        // ⭐ ÚJ TÉTELSZÁM → ajánlott határidő: mai nap
        ui->editDueDate->setDate(QDate::currentDate());//.addDays(1)
    }

    initializeBomModel(ref);

    if (_mode == DialogMode::Update) {
        setHeadEditable(true);
        setItemEditable(true);
    }

    QApplication::processEvents();
    layout()->activate();
    lblReferenceBig->updateGeometry();
    lblReferenceBig->raise();

}


ReferenceState AddInputDialog::getReferenceState(const QString& ref){
    auto* last = CuttingPlanRequestRegistry::instance().getLastRequest(ref);

    ReferenceState state;

    if (!last) {
        return ReferenceState::NewReference;
    }
    else if (last->materialId.isNull()) {
        return ReferenceState::HeadOnly;
    }
    else {
        return ReferenceState::FullRequest;
    }

}

void AddInputDialog::applyReferenceState(ReferenceState state)
{
    switch (state) {

    case ReferenceState::NewReference:
        setHeadEditable(true);
        setItemEditable(true);
        break;

    case ReferenceState::HeadOnly:
        setHeadEditable(false);
        setItemEditable(true);
        break;

    case ReferenceState::FullRequest:
        switch (_editMode) {

        case EditMode::None:
            // ⭐ Nézelődés
            setHeadEditable(false);
            setItemEditable(false);
            break;

        case EditMode::HeadEdit:
            // ⭐ HEAD szerkesztés
            setHeadEditable(true);
            setItemEditable(false);
            break;

        case EditMode::ItemEdit:
            // ⭐ Új anyag felvitele
            setHeadEditable(false);
            setItemEditable(true);
            break;
        }
        break;
    }
}


// void AddInputDialog::applyReferenceState(ReferenceState state){
//     switch (state) {
//     case ReferenceState::NewReference:
//         unlockAllFieldsAfterReference();
//         break;

//     case ReferenceState::HeadOnly:
//         setHeadEditable(false);
//         setItemEditable(true);
//         break;

//     case ReferenceState::FullRequest:
//         setHeadEditable(false);
//         break;
//     }

// }


void AddInputDialog::initializeBomModel(const QString& ref)
{
    // reset
    _bomModel.bomList.clear();
    _bomModel.missingList.clear();
    _bomModel.missingIndex = 0;

    // FIRST request kell a BOM generáláshoz
    auto* first = CuttingPlanRequestRegistry::instance().getFirstRequest(ref);
    if (!first)
        return;

    // 1) BOM lista generálása
    _bomModel.bomList = generateBomForRequest(*first);

    // 2) eddig felhasznált anyagok összegyűjtése
    QSet<QUuid> used;
    for (const auto& r : CuttingPlanRequestRegistry::instance().readAll()) {
        if (r.externalReference == ref)
            used.insert(r.materialId);
    }

    // 3) missing lista felépítése
    for (const QUuid& id : _bomModel.bomList) {
        if (!used.contains(id))
            _bomModel.missingList << id;
    }

    // 4) index reset
    _bomModel.missingIndex = 0;
}


QString AddInputDialog::computeNextItemNumber()
{
    QString current = ui->editReference->text().trimmed();
    if (current.isEmpty())
        return {};

    auto all = CuttingPlanRequestRegistry::instance().readAll();

    // --- 1) Papírkupac sorrend: registry sorrend ---
    QStringList refs;
    QSet<QString> seen;

    for (const auto& r : all) {
        if (!seen.contains(r.externalReference)) {
            seen.insert(r.externalReference);
            refs << r.externalReference;
        }
    }

    // --- 2) current első előfordulása ---
    int firstIdx = refs.indexOf(current);
    if (firstIdx < 0)
        return {};

    // --- 3) sorozat szerinti következő ---
    for (int i = firstIdx + 1; i < refs.size(); ++i) {
        if (refs[i] != current)
            return refs[i];
    }

    // --- 4) fallback: legnagyobb + 1 ---
    int maxRef = 0;
    for (const QString& r : refs) {
        bool ok = false;
        int val = r.toInt(&ok);
        if (ok && val > maxRef)
            maxRef = val;
    }

    return QString::number(maxRef + 1);
}

QVector<QUuid> AddInputDialog::generateBomForRequest(const Cutting::Plan::Request& req)
{
    QVector<QUuid> ordered;

    QHash<MaterialFamily, double> bomFamilies =
        BomRegistry::instance().bomMap(req.productTypeId, req.productSubtypeId);

    QVector<MaterialRole> roles = MaterialRoleRegistry::instance().findRoles(
        req.productTypeId, req.productSubtypeId);

    NamedColor reqColor = req.requiredColor;

    QList<MaterialFamily> famOrder;

    for (const auto& e : BomRegistry::instance().readAll()) {
        if (e.productTypeId == req.productTypeId &&
            e.productSubtypeId == req.productSubtypeId)
        {
            famOrder << e.family;
        }
    }

    QStringList famNames;
    for (MaterialFamily f : famOrder)
        famNames << MaterialFamilyUtils::toString(f);

    QString t = ProductTypeRegistry::instance().findById(req.productTypeId)->name;
    QString subt = ProductSubtypeRegistry::instance().findById(req.productSubtypeId)->name;

    qDebug() << "=== BOM DEBUG START ===";
    // qDebug() << "ProductType:" << t;
    // qDebug() << "ProductSubtype:" << subt;
    // qDebug() << "RequiredColor:" << reqColor.code();
     qDebug() << "famOrder:" << famNames;

    for (MaterialFamily fam : famOrder) {

        // qDebug() << "\n--- FAMILY:" <<  MaterialFamilyUtils::toString(fam) << "---";

        // 1) prefixek
        QStringList famPrefixes;
        for (const auto& role : roles) {
            if (role.family == fam) {
                QString prefix = role.barcodePrefix.trimmed();
                if (prefix.endsWith("*"))
                    prefix.chop(1);
                famPrefixes << prefix;
            }
        }

        //qDebug() << "Prefixes for family:" << famPrefixes;

        // 2) anyagok keresése
        bool found = false;

        for (const auto& prefix : famPrefixes) {

            //qDebug() << "  Checking prefix:" << prefix;

            for (const auto& mat : MaterialRegistry::instance().readAll()) {

                bool prefixMatch = matchPrefix(mat.barcode, prefix);
                bool familyMatch = (mat.family == fam);

                bool materialHasColor = mat.color.isValid() &&
                                        !mat.color.code().trimmed().isEmpty();
                bool colorMatch = (!materialHasColor ||
                                   mat.color.code() == reqColor.code());

                // qDebug() << "    Material:" << mat.barcode
                //          << "family:" << MaterialFamilyUtils::toString(mat.family)
                //          << "color:" << mat.color.code()
                //          << "prefixMatch:" << prefixMatch
                //          << "familyMatch:" << familyMatch
                //          << "colorMatch:" << colorMatch;

                if (!prefixMatch) continue;
                if (!familyMatch) continue;
                if (!colorMatch) continue;

                // ⭐ csak az első anyag a családból
                ordered << mat.id;
                // qDebug() << "    >>> SELECTED:" << mat.barcode;
                found = true;
                goto next_family;
            }
        }

        // if (!found)
        //     qDebug() << "    !!! NO MATERIAL FOUND FOR FAMILY !!!";

    next_family:;
    }

    QStringList oNames;
    for (auto& o : ordered)
        oNames << MaterialRegistry::instance().findById(o)->barcode;
    qDebug() << "Final BOM list:" << oNames;

    qDebug() << "\n=== BOM DEBUG END ===";


    return ordered;
}



QUuid AddInputDialog::computeNextMaterialForCurrentRef()
{
    if (_bomModel.missingList.isEmpty())
        return QUuid();   // nincs mit ajánlani

    QUuid next = _bomModel.missingList[_bomModel.missingIndex];

    _bomModel.missingIndex =
        (_bomModel.missingIndex + 1) % _bomModel.missingList.size();

    return next;
}


void AddInputDialog::loadHeadFields(const QString& ref)
{
    auto* last = CuttingPlanRequestRegistry::instance().getLastRequest(ref);
    if (last)
        applyFields_Head(*last);
}


void AddInputDialog::applyFields_Head(const Cutting::Plan::Request& r){
    applyOwnerFromRequest(r);
    applyDateFromRequest(r);
    applyColorFromRequest(r);
    applySurfaceFromRequest(r);
    applyProductTypeFromRequest(r);
    applyProductSubtypeFromRequest(r);

    // ⭐ quantity + handler side visszatöltése
    applyQuantityFromRequest(r);
    applySideFromRequest(r);
}

void AddInputDialog::applyFields_Item(const Cutting::Plan::Request& req){
    applyLengthFromRequest(req);
    applyMaterialFromRequest(req);
}

AddInputDialog::HeadFields AddInputDialog::headFromRegistry(const QString& ref) const
{
    HeadFields h;

    auto last = CuttingPlanRequestRegistry::instance().getLastRequest(ref);

    if (!last)
        return h; // üres, de minden mező inicializált

    h.owner = last->ownerName;
    h.due   = last->dueDate;
    h.color = last->requiredColor.code();
    h.surfaceCode = SurfaceTypeUtils::toCode(last->surface);
    h.typeId = last->productTypeId;
    h.subtypeId = last->productSubtypeId;

    h.quantity = last->quantity;
    h.leftCount = last->leftCount;
    h.rightCount = last->rightCount;

    return h;
}


AddInputDialog::HeadFields AddInputDialog::currentHeadFromDialog() const
{
    HeadFields h;

    h.owner = ui->editOwner->text().trimmed();
    h.due   = ui->editDueDate->date();
    h.color = ui->edit_Color->text().trimmed();
    h.surfaceCode = ui->comboBox_Surface->currentData().toString();
    h.typeId = selectedProductTypeId();
    h.subtypeId = selectedProductSubtypeId();

    h.quantity = ui->spinQuantity->value();

    if (h.quantity == 1) {
        h.leftCount  = ui->radioLeft->isChecked() ? 1 : 0;
        h.rightCount = ui->radioRight->isChecked() ? 1 : 0;
    } else {
        int left = ui->sliderHandler->value();
        h.leftCount = left;
        h.rightCount = h.quantity - left;
    }

    return h;
}


bool AddInputDialog::headFieldsDiffer(const HeadFields& a,
                                      const HeadFields& b) const
{
    return
        a.owner       != b.owner ||
        a.due         != b.due   ||
        a.color       != b.color ||
        a.surfaceCode != b.surfaceCode ||
        a.typeId      != b.typeId ||
        a.subtypeId   != b.subtypeId ||
        a.quantity    != b.quantity ||
        a.leftCount   != b.leftCount ||
        a.rightCount  != b.rightCount;
}



void AddInputDialog::updateHeadFieldsInRegistry(const QString& ref)
{
    AddInputDialog::HeadFields original = headFromRegistry(ref);
    AddInputDialog::HeadFields current  = currentHeadFromDialog();

    if (!headFieldsDiffer(original, current))
        return; // nincs változás → nincs frissítés

    auto r = CuttingPlanRequestRegistry::instance().getLastRequest(ref);

    if(r)
    {
        r->ownerName = current.owner;
        r->dueDate   = current.due;
        r->requiredColor = NamedColor::fromUserInput(current.color);
        r->surface = SurfaceTypeUtils::fromCode(current.surfaceCode);
        r->productTypeId = current.typeId;
        r->productSubtypeId = current.subtypeId;

        r->quantity = current.quantity;
        r->leftCount = current.leftCount;
        r->rightCount = current.rightCount;

        CuttingPlanRequestRegistry::instance().updateRequest(*r);
    }
}

