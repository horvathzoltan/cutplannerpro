#include "addinputdialog.h"

#include "../../../model/cutting/plan/request.h"
#include "model/cutting/plan/audit/naphalo_profile_postfix.h"
#include "product/material_role_utils.h"
#include "ui_addinputdialog.h"
#include "materials/registry/material_registry.h"
#include "view/common/layouts/qflowlayout.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"

#include <QCompleter>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <common/eventlogger.h>
#include <materials/model/material_master.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>
#include <product/registry/productattributeregistry.h>
#include <product/selector/material_selector.h>

QString AddInputDialog::s_lastExternalRef;
QSet<QString> AddInputDialog::s_ownerCache;

bool AddInputDialog::s_lastRepeat = false;

AddInputDialog::AddInputDialog(QWidget *parent,
                               DialogMode mode,
                               const Cutting::Plan::Request* initial)
    : QDialog(parent)
    , ui(new Ui::AddInputDialog)
    //, _matrix(matrix)
    , current_requestId(QUuid::createUuid())

{
    _editMode = EditMode::None;

    if (s_ownerCache.isEmpty()) {
        loadOwnerCache();
    }

    _mode = mode;
    _shiftEnterAccepted = false;

    ui->setupUi(this);

    ui->stackedWidget_stackSubtype->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->stackHandlerSide->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

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
        connect(rb, &QRadioButton::toggled, this, [this](bool checked){
            if (!checked) return;

            refreshBom();
            onProductTypeChanged(true);   // megmarad a subtype stack váltás
        });

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

            connect(rb, &QRadioButton::toggled, this, [this](bool checked){
                if (!checked) return;

                refreshBom();
                updateAttributePanel();
            });

        }

        subtypeStack->addWidget(page);
    }

    ui->editLength->installEventFilter(this);

    auto completer = new QCompleter(s_ownerCache.values(), this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->editOwner->setCompleter(completer);

    populateMaterialCombo();

    populateSurfaceCombo();
    //connect(ui->edit_Color, &QLineEdit::textChanged, this, &AddInputDialog::updateColorPreview);
    connect(ui->comboMaterial, qOverload<int>(&QComboBox::currentIndexChanged), this, &AddInputDialog::updateColorPreview);
//    connect(ui->editLength, &QLineEdit::textChanged, this, &AddInputDialog::updateColorPreview);
    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged), this, &AddInputDialog::updateColorPreview);


    // ⭐ Tiszta tételszám ajánlás
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
        if (ref.isEmpty()){
            zInfo("ref is null");
            return;
        }

        QUuid nextMat = computeNextMaterialForCurrentRef();
        if (nextMat.isNull()){
            zInfo("nextMat is null");
            return;
        }

        int idx = ui->comboMaterial->findData(nextMat);
        if (idx >= 0){
            auto mat = MaterialRegistry::instance().findById(nextMat);

            QString matName = mat?mat->toReportLabel():"?";
            zInfo("nextMat:" + matName);

            _editMode = EditMode::ItemEdit;
            applyReferenceState(ReferenceState::FullRequest);
            ui->comboMaterial->setCurrentIndex(idx);
            ui->editLength->clear();
            ui->editLength->setFocus();
        } else{
            zInfo("nextMat nem található a comboban ");

        }
    });

    // qty változás → handler‑UI váltás
    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged),
            this, &AddInputDialog::onQuantityChanged);

    // slider változás → label frissítés
    connect(ui->sliderHandler, &QSlider::valueChanged,
            this, &AddInputDialog::updateSliderLabels);


    connect(ui->btnEditHead, &QPushButton::clicked, this, [this]() {
        if(_editMode != EditMode::HeadEdit){
            _editMode = EditMode::HeadEdit;
        } else{
            _editMode = EditMode::None;
        }
        //loadReference(ui->editReference->text().trimmed());
        applyReferenceState(getReferenceState(ui->editReference->text().trimmed()));
    });

    connect(ui->btnEditItem, &QPushButton::clicked, this, [this]() {
        if(_editMode != EditMode::ItemEdit){
            _editMode = EditMode::ItemEdit;
        } else{
            _editMode = EditMode::None;
        }

        applyReferenceState(getReferenceState(ui->editReference->text().trimmed()));
        //loadReference(ui->editReference->text().trimmed());
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


    // --- LENGTH DEBOUNCE ---
    _lengthDebounceTimer = new QTimer(this);
    _lengthDebounceTimer->setSingleShot(true);
    _lengthDebounceTimer->setInterval(2000);

    connect(_lengthDebounceTimer, &QTimer::timeout, this, [this](){
        updateColorPreview();
        refreshBom();
    });

    connect(ui->spinBox_width, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int){
                _lengthDebounceTimer->start();
            });

    // connect(ui->spinBox_height, qOverload<int>(&QSpinBox::valueChanged),
    //         this, [this](int){
    //             refreshBom();
    //         });

    // connect(ui->editLength, &QLineEdit::textChanged,
    //         this, [this](const QString& txt){
    //             bool ok = false;
    //             int L = txt.toInt(&ok);

    //             if (ok) {
    //                 _lengthHint = L;
    //                 _lengthDebounceTimer->start();   // csak akkor indul, ha van érték
    //             } else {
    //                 // ❗ Üres vagy érvénytelen → NEM indítunk BOM-frissítést
    //                 // ❗ Így a NextMaterial nem fut kétszer
    //             }
    //         });


    // --- COLOR DEBOUNCE ---
    _colorDebounceTimer = new QTimer(this);
    _colorDebounceTimer->setSingleShot(true);
    _colorDebounceTimer->setInterval(2000);

    connect(_colorDebounceTimer, &QTimer::timeout, this, [this](){
        updateColorPreview();
        refreshBom();
    });

    connect(ui->edit_Color, &QLineEdit::textChanged,
            this, [this](){
                _colorDebounceTimer->start();    // ❗ csak timer, NINCS azonnali BOM
            });


    // ⭐ Induló inicializálás
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
        updateAttributePanel();

    });

    //groupboxAttributes_hide();
}

AddInputDialog::~AddInputDialog()
{
    saveOwnerCache();
    delete ui;
}

void AddInputDialog::groupboxAttributes_hide(){
    bool v = ui->groupBox_attributes->isVisible();

    zInfo(L("groupboxAttributes_hide:")+(v?"true":"false"));
    if (!v)
        return;   // már rejtve → nincs teendő

    int h = ui->groupBox_attributes->sizeHint().height();
    ui->groupBox_attributes->hide();
    this->resize(this->width(), this->height() - h);
}

void AddInputDialog::groupboxAttributes_show(){
    bool v = ui->groupBox_attributes->isVisible();

    zInfo(L("groupboxAttributes_show:")+(v?"true":"false"));

    if (v)
        return;   // már látszik → nincs teendő

    int h = ui->groupBox_attributes->sizeHint().height();
    ui->groupBox_attributes->show();
    this->resize(this->width(), this->height() + h);
}

void AddInputDialog::refreshBom()
{
    zInfo("refreshBom() called");

    {
        QUuid id = _bomModel.lastSuggestedMaterial;
        if (id.isNull()) {
            zInfo("  lastSuggestedMaterial at entry: NULL");
        } else {
            const MaterialMaster* m = MaterialRegistry::instance().findById(id);
            if (m)
                zInfo(QString("  lastSuggestedMaterial at entry: %1 [%2]")
                          .arg(m->name)
                          .arg(m->barcode));
            else
                zInfo(QString("  lastSuggestedMaterial at entry: UNKNOWN GUID %1")
                          .arg(id.toString()));
        }
    }

    // 1) User által bevitt request (érintetlen!)
    Cutting::Plan::Request req = getModel();

    //2) Selector input: req + hint (lokális kontextus)
    // Cutting::Plan::Request selReq = req;
    // if (selReq <= 0 && _lengthHint > 0)
    //     selReq.requiredLength = _lengthHint;


    //zInfo("selReq.requiredLength: "+QString::number(req.requiredLength));

    // 3) Tiszta BOM generálás
    auto bom = MaterialRegistry::instance().generateBom(
        req.productTypeId,
        req.productSubtypeId
        );

    // 4) Preferencia alkalmazása (jobbágy)
    auto ranked = MaterialSelector::rankMaterials(bom, req);

    // 5) BOM utófeldolgozás (király)
    QMap<MaterialRole, QUuid> bestPerRole;

    for (auto id : ranked) {
        const MaterialMaster* m = MaterialRegistry::instance().findById(id);
        if (!m) continue;

        MaterialRole role = MaterialRoleUtils::makeRole(req, m);

        if (!bestPerRole.contains(role))
            bestPerRole[role] = id;
    }



    QVector<QUuid> recommended;
    for (auto id : ranked) {
        const MaterialMaster* m = MaterialRegistry::instance().findById(id);
        if (!m) continue;

        MaterialRole role = MaterialRoleUtils::makeRole(req, m);

        if (bestPerRole[role] == id)
            recommended << id;

    }

    // 1) BOM ajánlás beállítása
    _bomModel.bomList = recommended;

    // 2) lastSuggestedMaterial szinkronizálása az új BOM-hoz

    // ranked-first fallback
    auto pickRankedFirst = [&]() -> QUuid {
        for (auto id : ranked) {
            if (_bomModel.bomList.contains(id))
                return id;
        }
        return QUuid();
    };

    // 2) lastSuggestedMaterial szinkronizálása az új BOM-hoz
    if (!_bomModel.lastSuggestedMaterial.isNull()) {
        // ha kiesett → ranked első BOM elem
        if (!_bomModel.bomList.contains(_bomModel.lastSuggestedMaterial)) {
            _bomModel.lastSuggestedMaterial = pickRankedFirst();
        }
    }
    else {
        // ha eddig nem volt → ranked első szabad BOM elem
        for (auto id : ranked) {
            if (_bomModel.bomList.contains(id) &&
                !_bomModel.addedMaterials.contains(id))
            {
                _bomModel.lastSuggestedMaterial = id;
                break;
            }
        }
    }

    zInfo("=== Recommended BOM order ===");
    for (auto id : _bomModel.bomList) {
        const MaterialMaster* m = MaterialRegistry::instance().findById(id);
        if (m)
            zInfo(QString("  %1  [%2]").arg(m->name).arg(m->barcode));
    }
    zInfo("=== END Recommended BOM ===");

    // 3) ComboMaterial szinkronizálása a lastSuggestedMaterial-hez
    // if (!_bomModel.lastSuggestedMaterial.isNull()) {
    //     int idx = ui->comboMaterial->findData(_bomModel.lastSuggestedMaterial);
    //     if (idx >= 0) {
    //         ui->comboMaterial->setCurrentIndex(idx);
    //         zInfo(QString("comboMaterial synced to %1")
    //                   .arg(_bomModel.lastSuggestedMaterial.toString()));
    //     } else {
    //         zInfo("comboMaterial: lastSuggestedMaterial not found in combo");
    //     }
    // }

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

    req.fullWidth_mm = ui->spinBox_width->value();
    req.fullHeight_mm = ui->spinBox_height->value();

    req.attributes.clear();

    // Ha a terméktípushoz van attribútum, akkor kiolvassuk
    auto attrs = ProductAttributeRegistry::instance().getAll(
        currentProductTypeCode(),
        currentProductSubtypeCode());

    if (attrs.contains("meghajtas")) {
        if (ui->radioAttrMotoros->isChecked())
            req.attributes["meghajtas"] = "motoros";
        else if (ui->radioAttrKurblis->isChecked())
            req.attributes["meghajtas"] = "kurblis";

        zInfo("attr kiolvasva, meghajtas: " + req.attributes["meghajtas"]);
    } else {
        zInfo("attr NINCS kiolvasva");
    }

    return req;
}

QString AddInputDialog::currentProductTypeCode() const
{
    auto id = selectedProductTypeId();
    auto* type = ProductTypeRegistry::instance().findById(id);
    return type ? type->code : QString();
}

QString AddInputDialog::currentProductSubtypeCode() const
{
    auto id = selectedProductSubtypeId();
    auto* st = ProductSubtypeRegistry::instance().findById(id);
    return st ? st->code : QString("*");
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

    if (req.fullWidth_mm <= 0)
        errors << "A termék szélessége nincs megadva.";

    if (req.fullHeight_mm <= 0)
        errors << "A termék magassága nincs megadva.";

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

    _bomModel.addedMaterials.insert(req.materialId);


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
    _suppressPreview = true;
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
}

void AddInputDialog::applyWidth(const Cutting::Plan::Request& req){
        ui->spinBox_width->setValue(req.fullWidth_mm);
}

void AddInputDialog::applyHeight(const Cutting::Plan::Request& req){
    ui->spinBox_height->setValue(req.fullHeight_mm);
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

    updateAttributePanel();
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
    QString postfix = (mat ? ProfileUtils::profilePostfixFor(mat->barcode) : QString());

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

void AddInputDialog::on_btnEditReference_clicked(bool checked)
{
    Q_UNUSED(checked);
    enterReferenceEditMode();
}

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

    auto* last = CuttingPlanRequestRegistry::instance().getLastRequest(ref);
    auto state = getReferenceState(ref);
    applyReferenceState(state);

    if(last){
        // ❗ csak akkor töltsük vissza, ha NEM Update módban vagyunk
        if (_mode != DialogMode::Update) {
            applyRequestToWidgets(*last);
        }
        // ⭐ HOSSZ HINT: ha már volt anyag → registry a forrás
        // if (last->requiredLength > 0)
        //     _lengthHint = last->requiredLength;

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


void AddInputDialog::initializeBomModel(const QString& ref)
{
    // 0) BOM state reset
    _bomModel.bomList.clear();
    _bomModel.addedMaterials.clear();
    _bomModel.lastSuggestedMaterial = QUuid();

    // 1) BOM generálása az aktuális UI állapot alapján
    refreshBom();   // ez tölti fel _bomModel.bomList-et (recommended)

    // 2) Registry lekérdezés — mi lett már hozzáadva ehhez a ref-hez?
    auto existing = CuttingPlanRequestRegistry::instance().findByExternalReference(ref);
    for (const auto& req : existing) {
        if (!req.materialId.isNull())
            _bomModel.addedMaterials.insert(req.materialId);
    }

    // 3) lastSuggestedMaterial inicializálása:
    // első olyan BOM elem, ami nincs addedMaterials-ben
    for (const auto& id : _bomModel.bomList) {
        if (!_bomModel.addedMaterials.contains(id)) {
            _bomModel.lastSuggestedMaterial = id;
            return;
        }
    }

    // 4) ha minden hozzá van adva → nincs ajánlás
    _bomModel.lastSuggestedMaterial = QUuid();
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


QUuid AddInputDialog::computeNextMaterialForCurrentRef()
{
    zInfo("computeNextMaterialForCurrentRef() called");
    {
        QUuid id = _bomModel.lastSuggestedMaterial;
        if (id.isNull()) {
            zInfo("  lastSuggestedMaterial BEFORE: NULL");
        } else {
            const MaterialMaster* m = MaterialRegistry::instance().findById(id);
            if (m)
                zInfo(QString("  lastSuggestedMaterial BEFORE: %1 [%2]")
                          .arg(m->name)
                          .arg(m->barcode));
            else
                zInfo(QString("  lastSuggestedMaterial BEFORE: UNKNOWN GUID %1")
                          .arg(id.toString()));
        }
    }

    zInfo("  addedMaterials size: " + QString::number(_bomModel.addedMaterials.size()));

    zInfo("  BOM list in computeNextMaterial:");
    for (auto id : _bomModel.bomList) {
        const MaterialMaster* m = MaterialRegistry::instance().findById(id);
        if (m)
            zInfo(QString("    %1 [%2]").arg(m->name).arg(m->barcode));
    }

    // 0) Ha nincs BOM → nincs ajánlás
    if (_bomModel.bomList.isEmpty())
        return QUuid();

    // 1) Ha nincs lastSuggestedMaterial → keressük meg az első olyan BOM elemet,
    //    ami még nincs addedMaterials-ben
    if (_bomModel.lastSuggestedMaterial.isNull()) {
        for (const auto& id : _bomModel.bomList) {
            if (!_bomModel.addedMaterials.contains(id)) {
                _bomModel.lastSuggestedMaterial = id;
                zInfo("  FIRST pick (no lastSuggestedMaterial): " + id.toString());
                return id;
            }
        }
        zInfo("  No candidate found (all added)");
        return QUuid(); // minden hozzá van adva
    }

    // ⭐ ÚJ LOGIKA: ha a lastSuggestedMaterial még nincs addedMaterials-ben,
    // akkor EZ az aktuális jelölt → NEM szabad átugrani
    if (!_bomModel.addedMaterials.contains(_bomModel.lastSuggestedMaterial)) {

        auto * mat = MaterialRegistry::instance().findById(_bomModel.lastSuggestedMaterial);
        QString matName = mat?mat->toReportLabel():"?";
        zInfo("  USING current lastSuggestedMaterial (not yet added): " + matName);
        return _bomModel.lastSuggestedMaterial;
    }

    // 2) Ha van lastSuggestedMaterial ÉS már hozzá lett adva →
    // keressük meg a BOM-ban, és lépjünk tovább
    int idx = _bomModel.bomList.indexOf(_bomModel.lastSuggestedMaterial);
    zInfo("  indexOf(lastSuggestedMaterial) in BOM: " + QString::number(idx));

    int start = (idx >= 0 ? idx + 1 : 0);

    // 3) Körbejárás ID-alapon
    for (int i = 0; i < _bomModel.bomList.size(); ++i) {
        int pos = (start + i) % _bomModel.bomList.size();
        QUuid candidate = _bomModel.bomList[pos];

        if (!_bomModel.addedMaterials.contains(candidate)) {
            _bomModel.lastSuggestedMaterial = candidate;
            return candidate;
        }
    }

    // 4) Ha minden hozzá van adva → nincs ajánlás
    return QUuid();
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

    applyWidth(r);
    applyHeight(r);
    applyAttributes(r);
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

void AddInputDialog::updateAttributePanel()
{
    //zTrace();

    bool wasVisible = ui->groupBox_attributes->isVisible();
    //zInfo("ATTR: wasVisible = " + QString(wasVisible ? "true" : "false"));

    auto* type = ProductTypeRegistry::instance().findById(selectedProductTypeId());
    auto* subtype = ProductSubtypeRegistry::instance().findById(selectedProductSubtypeId());

    QString typeName = type ? type->name : "N/A";
    QString subtypeName = subtype ? subtype->name : "N/A";

    //zInfo("ATTR: selected typeId = " + typeName);
    //zInfo("ATTR: selected subtypeId = " + subtypeName);

    // --- 1) nincs type → panel eltűnik ---
    if (!type) {
        //zInfo("ATTR: type == nullptr → hide panel");
        if (wasVisible)
            groupboxAttributes_hide();
        return;
    }

    QString typeCode = type->code;
    QString subtypeCode = subtype ? subtype->code : "*";

    //zInfo("ATTR: typeCode = " + typeCode);
    //zInfo("ATTR: subtypeCode = " + subtypeCode);

    auto attrs = ProductAttributeRegistry::instance().getAll(typeCode, subtypeCode);

    //zInfo("ATTR: attrs keys = " + QStringList(attrs.keys()).join(", "));

    // --- 2) nincs attribútum → panel eltűnik ---
    if (!attrs.contains("meghajtas")) {
      //  zInfo("ATTR: meghajtas NOT found → hide panel");
        if (wasVisible)
            groupboxAttributes_hide();
        return;
    }

    // --- 3) van attribútum → panel megjelenik ---
    if (!wasVisible)
        groupboxAttributes_show();

    // ❗ NINCS rádióállítás itt
    // applyAttributes fogja beállítani, ha van Request-érték
    // ha nincs, akkor ő dönt a default-ról
}

void AddInputDialog::applyAttributes(const Cutting::Plan::Request& r)
{
    zTrace();

    // 1) Ha a type/subtype-hoz nincs attribútum → panel rejtve marad
    auto* type = ProductTypeRegistry::instance().findById(r.productTypeId);
    auto* subtype = ProductSubtypeRegistry::instance().findById(r.productSubtypeId);

    if (!type) {
        ui->groupBox_attributes->hide();
        return;
    }

    QString typeCode = type->code;
    QString subtypeCode = subtype ? subtype->code : "*";

    auto attrs = ProductAttributeRegistry::instance().getAll(typeCode, subtypeCode);
    if (!attrs.contains("meghajtas")) {
        ui->groupBox_attributes->hide();
        return;
    }

    ui->groupBox_attributes->show();

    // 2) Ha a Request-ben van érték → AZ élvez prioritást
    QString v;
    if (r.attributes.contains("meghajtas")) {
        v = r.attributes["meghajtas"];
    } else {
        // 3) Ha nincs Request-érték → registry default
        v = attrs["meghajtas"];
    }

    if (v == "motoros")
        ui->radioAttrMotoros->setChecked(true);
    else
        ui->radioAttrKurblis->setChecked(true);

    zInfo("applyAttributes: meghajtas = " + v);
}



