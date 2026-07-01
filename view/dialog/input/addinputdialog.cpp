#include "addinputdialog.h"

#include "../../../model/cutting/plan/request.h"
#include "ui_addinputdialog.h"
#include "materials/registry/material_registry.h"
#include "view/common/layouts/qflowlayout.h"
#include "view/dialog/materialsearch/materialsearchdialog.h"

#include <QCompleter>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <common/eventlogger.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

QString AddInputDialog::s_lastExternalRef;
QMap<QString, AddInputDialog::RequestContext> AddInputDialog::_contexts;
QSet<QString> AddInputDialog::s_ownerCache;

bool AddInputDialog::s_lastRepeat = false;

AddInputDialog::AddInputDialog(QWidget *parent,
                               DialogMode mode,
                               const Cutting::Plan::Request* initial)
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

    // ⭐ Tiszta tételszám ajánlás
    _nextSuggestedRef = computeNextReference();

    // ⭐ Event binding
    // connect(ui->editReference, &QLineEdit::textChanged,
    //         this, [this](const QString& ref) {

    //             _contextMode = detectContextMode(ref);
    //             applyContextMode(_contextMode, ref);
    //             updateContextModeLabel();
    //         });

    connect(ui->editReference, &QLineEdit::textEdited,
            this, [this]() {
                _contextMode = ContextMode::Editing;
                updateContextModeLabel();
            });

    connect(ui->editReference, &QLineEdit::editingFinished,
            this, [this]() {
                // ⭐ UPDATE módban nincs context workflow
                if (_contextMode == ContextMode::Update)
                    return;

                QString ref = ui->editReference->text().trimmed();

                // ⭐ 1) üres → nincs workflow
                if (ref.isEmpty())
                    return;

                // ⭐ 2) nem változott → nincs workflow
                if (ref == _originalReference)
                    return;

                // ⭐ 3) változott → workflow indul
                _contextMode = detectContextMode(ref);
                applyContextMode(_contextMode, ref);
                updateContextModeLabel();

                // ⭐ 4) új eredeti érték mentése
                _originalReference = ref;
            });



    // qty változás → handler‑UI váltás
    connect(ui->spinQuantity, qOverload<int>(&QSpinBox::valueChanged),
            this, &AddInputDialog::onQuantityChanged);

    // slider változás → label frissítés
    connect(ui->sliderHandler, &QSlider::valueChanged,
            this, &AddInputDialog::updateSliderLabels);


    // ⭐ MODE-AWARE INITIALIZATION
    if (mode == DialogMode::Update && initial) {
        // 1) Azonosító
        current_requestId = initial->requestId;

        // 2) Mezők visszatöltése
        applyRequestToWidgets(*initial);

        // 3) Kontextus mód
        _contextMode = ContextMode::Update;
        updateContextModeLabel();

        // 4) Nem szerkeszthető mezők pedig NINCSENEK
        // azaz
        // MINDEN mező szerkeszthető
        setContextEditable(true);

        ui->chk_Repeat->hide();   // ⭐ UPDATE módban nincs sorozatbevitel

        // 5) Fókusz
        applyInitialFocus();

        return;   // ❗ NINCS initializeDialog()
    }

    // ⭐ Induló inicializálás
    QTimer::singleShot(0, this, [this]() {
        initializeDialog();
        ui->chk_Repeat->setChecked(s_lastRepeat);   // ⭐ repeat visszatöltése
        _originalReference = ui->editReference->text().trimmed();   // ⭐ eredeti érték mentése
    });

}

AddInputDialog::~AddInputDialog()
{
    saveOwnerCache();
    saveContextMap();
    delete ui;
}

// void AddInputDialog::initializeDialog()
// {
//     // ⭐ Ha van nextRef → Sequential indul
//     if (!_nextSuggestedRef.isEmpty()) {
//         ui->editReference->setText(_nextSuggestedRef);
//         _contextMode = ContextMode::Sequential;
//         applySequentialContext(_nextSuggestedRef);
//     }
//     else {
//         // ⭐ Ha nincs → NewOrder indul
//         ui->editReference->clear();
//         _contextMode = ContextMode::NewOrder;
//         applyNewOrderContext();
//     }

//     _contextMode = detectContextMode(ui->editReference->text());
//     applyContextMode(_contextMode, ui->editReference->text());
//     updateContextModeLabel();
//     applyInitialFocus();
// }

// void AddInputDialog::initializeDialog()
// {
//     //
//     // 1) CREATE mód → teljesen üres indulás
//     //
//     if (_mode == DialogMode::Create) {
//         ui->editReference->clear();
//         _contextMode = ContextMode::NewOrder;

//         // teljes reset
//         applyNewOrderContext();
//         updateContextModeLabel();
//         applyInitialFocus();
//         return;
//     }


//     //
//     // 2) UPDATE mód → már korábban visszatöltöttük a mezőket
//     //    initializeDialog() nem fut Update módban (lásd konstruktor)
//     //
//     //    → ide nem jutunk el Update módban
//     //


//     //
//     // 3) NEM CREATE mód → sorozat vagy kézi tételszám workflow
//     //
//     if (!_nextSuggestedRef.isEmpty()) {
//         // Sorozat mód
//         ui->editReference->setText(_nextSuggestedRef);
//         _contextMode = ContextMode::Sequential;

//         applySequentialContext(_nextSuggestedRef);
//     }
//     else {
//         // Új megrendelő workflow
//         ui->editReference->clear();
//         _contextMode = ContextMode::NewOrder;

//         applyNewOrderContext();
//     }

//     //
//     // 4) Kontextus workflow lefuttatása
//     //
//     QString ref = ui->editReference->text().trimmed();
//     _contextMode = detectContextMode(ref);
//     applyContextMode(_contextMode, ref);

//     updateContextModeLabel();
//     applyInitialFocus();
// }

void AddInputDialog::initializeDialog()
{
    //
    // 1) CREATE mód
    //
    if (_mode == DialogMode::Create) {

        // ⭐ Ha van előző tétel és repeat be volt kapcsolva,
        //    és van érvényes nextRef → SOROZAT mód induljon
        if (s_lastRepeat && !_nextSuggestedRef.isEmpty()) {

            // következő tételszám beírása
            ui->editReference->setText(_nextSuggestedRef);
            _contextMode = ContextMode::Sequential;

            // előző tétel kontextusának visszatöltése
            applySequentialContext(_nextSuggestedRef);

            // context workflow lefuttatása
            QString ref = ui->editReference->text().trimmed();
            _contextMode = detectContextMode(ref);
            applyContextMode(_contextMode, ref);

            updateContextModeLabel();
            applyInitialFocus();
            return;
        }

        // ⭐ Ha nincs repeat vagy nincs nextRef → ÚJ MEGRENDELŐ
        ui->editReference->clear();
        _contextMode = ContextMode::NewOrder;

        applyNewOrderContext();
        updateContextModeLabel();
        applyInitialFocus();
        return;
    }

    //
    // 2) NEM CREATE mód → sorozat vagy kézi tételszám workflow
    //
    if (!_nextSuggestedRef.isEmpty()) {
        ui->editReference->setText(_nextSuggestedRef);
        _contextMode = ContextMode::Sequential;

        applySequentialContext(_nextSuggestedRef);
    }
    else {
        ui->editReference->clear();
        _contextMode = ContextMode::NewOrder;

        applyNewOrderContext();
    }

    QString ref = ui->editReference->text().trimmed();
    _contextMode = detectContextMode(ref);
    applyContextMode(_contextMode, ref);

    updateContextModeLabel();
    applyInitialFocus();
}


AddInputDialog::ContextMode AddInputDialog::detectContextMode(const QString& ref)
{
    QString trimmed = ref.trimmed();

    if (_contexts.contains(trimmed))
        return ContextMode::Existing;

    bool okLast = false, okCurr = false;
    int last = s_lastExternalRef.toInt(&okLast);
    int curr = trimmed.toInt(&okCurr);

    if (okLast && okCurr && curr == last + 1)
        return ContextMode::Sequential;

    return ContextMode::NewOrder;
}

void AddInputDialog::applyContextMode(ContextMode mode, const QString& ref)
{
    switch (mode) {
    case ContextMode::Existing:
        applyExistingContext(ref);
        break;

    case ContextMode::Sequential:
        applySequentialContext(ref);
        break;

    case ContextMode::NewOrder:
        applyNewOrderContext();
        break;
    }
}

void AddInputDialog::applyExistingContext(const QString& ref)
{
    auto it = _contexts.find(ref.trimmed());
    if (it != _contexts.end()) {
        applyContextToWidgets(it.value());
        setContextEditable(false);
    }
}

void AddInputDialog::applySequentialContext(const QString& ref)
{
    auto it = _contexts.find(s_lastExternalRef);
    if (it != _contexts.end()) {
        applyContextToWidgets(it.value());
    }

    // ❌ nextRef automatikus beírása TILOS
    // (ha a user akarja, majd beírja)

    // ⭐ Automatikus következő tételszám
    // Sequential mód → csak akkor írjuk be a nextRef-et,
    // ha a user nem írt be explicit értéket
    // if (ref.trimmed().isEmpty()) {
    //     QString nextRef = computeNextReference();
    //     if (!nextRef.isEmpty()) {
    //         ui->editReference->setText(nextRef);
    //     }
    // }


    resetForSequential();
    setContextEditable(true);
}


void AddInputDialog::applyNewOrderContext()
{
    // ⭐ Új megrendelő → referencia mező törlése
    //ui->editReference->clear(); // ❌ TILOS

    resetForNewOrder();
    setContextEditable(true);
}


void AddInputDialog::resetForNewOrder()
{
    ui->editOwner->clear();

    // ⭐ Dátum → holnap
    ui->editDueDate->setDate(QDate::currentDate().addDays(1));

    ui->edit_Color->clear();
    ui->comboBox_Surface->setCurrentIndex(-1);   // ⭐ surface törlése

    ui->editLength->clear();
    ui->spinQuantity->setValue(1);

    ui->radioLeft->setAutoExclusive(false);
    ui->radioRight->setAutoExclusive(false);

    ui->radioLeft->setChecked(false);
    ui->radioRight->setChecked(false);

    ui->radioLeft->setAutoExclusive(true);
    ui->radioRight->setAutoExclusive(true);
    ui->sliderHandler->setValue(0);
}

void AddInputDialog::resetForSequential()
{
    // ⭐ Dátum → holnap
    //ui->editDueDate->setDate(QDate::currentDate().addDays(1));

    ui->editLength->clear();
    //ui->spinQuantity->setValue(1);

    ui->radioLeft->setAutoExclusive(false);
    ui->radioRight->setAutoExclusive(false);

    ui->radioLeft->setChecked(false);
    ui->radioRight->setChecked(false);

    ui->radioLeft->setAutoExclusive(true);
    ui->radioRight->setAutoExclusive(true);
    ui->sliderHandler->setValue(0);

    // ⭐ NEM törlünk színt és surface-t
    // ui->edit_Color->clear();        // ❌ TILOS
    // ui->comboBox_Surface->setCurrentIndex(-1); // ❌ TILOS
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

/**/

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

    // ⭐ ProductType (getter)
    req.productTypeId = selectedProductTypeId();

    // ⭐ ProductSubtype (getter — NEM mezőből!)
    req.productSubtypeId = selectedProductSubtypeId();

    req.dueDate = ui->editDueDate->date();

    req.color = ui->edit_Color->text().trimmed();

    QString surfaceCode = ui->comboBox_Surface->currentData().toString();
    req.surface = SurfaceTypeUtils::fromCode(surfaceCode);

    return req;
}



bool AddInputDialog::validateInputs() {

    Cutting::Plan::Request req = getModel();
    QStringList errors;

    // 1) J/B darabszám
    const int totalPieces = req.quantity;
    const int l = req.leftCount;
    const int r = req.rightCount;

    if ((l + r != totalPieces) && !(l == 0 && r == 0)) {
        errors << "A balos és jobbos darabszám összege nem egyezik meg a teljes darabszámmal.";
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

    const QString ref = req.externalReference;
    s_lastExternalRef = ref;
    s_lastRepeat = ui->chk_Repeat->isChecked();

    RequestContext ctx;
    ctx.ownerName        = req.ownerName;
    ctx.dueDate          = req.dueDate;

    // ⭐ ProductType / ProductSubtype getterből
    ctx.productTypeId    = req.productTypeId;
    ctx.productSubtypeId = req.productSubtypeId;

    // ⭐ Side → a Request már kiszámolta
    ctx.side             = (req.leftCount > 0 ? HandlerSide::Left
                                  : HandlerSide::Right);

    ctx.defaultMaterialId = req.materialId;
    ctx.color             = req.color;

    auto surfaceCode= SurfaceTypeUtils::toCode(req.surface);
    ctx.surfaceCode = surfaceCode;

    _contexts[ref] = ctx;
    s_ownerCache.insert(req.ownerName);

    QDialog::accept();
}


// void AddInputDialog::setModel(const Cutting::Plan::Request& req)
// {
//     // ⭐ 1) Azonosító beállítása
//     current_requestId = req.requestId;

//     // ⭐ 2) UI mezők feltöltése modulárisan
//     applyRequestToWidgets(req);

//     // ⭐ 3) Update mód → Existing context
//     _contextMode = ContextMode::Existing;
//     updateContextModeLabel();

//     // ⭐ 4) Update módban nem szerkeszthető
//     setContextEditable(false);

//     // ⭐ 5) Fókusz beállítása
//     applyInitialFocus();
// }



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

void AddInputDialog::applyContextToWidgets(const RequestContext& ctx)
{
    applyOwnerAndDate(ctx);

    applyProductTypeFromContext(ctx);
    applyProductSubtypeFromContext(ctx);

    applySideFromContext(ctx);
    applyMaterialFromContext(ctx);
    applyColorFromContext(ctx);

    applySurfaceFromContext(ctx);
}


void AddInputDialog::applyRequestToWidgets(const Cutting::Plan::Request& req)
{
    applyOwnerFromRequest(req);
    applyReferenceFromRequest(req);
    applyDateFromRequest(req);
    applyColorFromRequest(req);

    applyProductTypeFromRequest(req);
    applyProductSubtypeFromRequest(req);

    applySideFromRequest(req);
    applyMaterialFromRequest(req);
    applyLengthAndQuantityFromRequest(req);

    applySurfaceFromRequest(req);   // ⭐ HIÁNYZÓ, MOST KELL
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
    // ⭐ Reset mindig új megrendelő workflow-t indít
    _contextMode = ContextMode::NewOrder;
    updateContextModeLabel();

    // ⭐ Referencia mező törlése
    ui->editReference->clear();
    _originalReference.clear();

    // ⭐ Reset modul használata
    resetForNewOrder();

    // ⭐ Szerkeszthető mezők engedélyezése
    setContextEditable(true);

    // ⭐ Fókusz beállítása
    applyInitialFocus();

    s_lastRepeat = false;
}


void AddInputDialog::loadContextMap()
{
    QFile f(CONTEXT_CACHE_FN);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty()) continue;

        //externalRef ; ownerName ; dueDate ; productTypeId ; productSubtypeId ; side ; materialBarcode ; color
        auto parts = line.split(';');
        if (parts.size() < 9)
            continue;

        RequestContext ctx;
        ctx.ownerName = parts[1];
        ctx.dueDate = QDate::fromString(parts[2], "yyyy-MM-dd");
        // ctx.productTypeId    = QUuid(parts[3]);
        // ctx.productSubtypeId = QUuid(parts[4]);

        QString typeCode = parts[3].trimmed();
        QString subtypeCode = parts[4].trimmed();

        auto* type = ProductTypeRegistry::instance().findByCode(typeCode);
        auto* subtype = ProductSubtypeRegistry::instance().findByCode(subtypeCode);

        ctx.productTypeId    = type ? type->id : QUuid();
        ctx.productSubtypeId = subtype ? subtype->id : QUuid();

        ctx.side = HandlerSideUtils::parse(parts[5]);

        auto matBarcode = parts[6];
        auto* mat = MaterialRegistry::instance().findByBarcode(matBarcode);
        ctx.defaultMaterialId = mat?mat->id:QUuid();

        ctx.color = parts[7];
        ctx.surfaceCode = parts[8].trimmed();

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

        auto* type = ProductTypeRegistry::instance().findById(ctx.productTypeId);
        auto* subtype = ProductSubtypeRegistry::instance().findById(ctx.productSubtypeId);

        QString typeCode = type ? type->code : "";
        QString subtypeCode = subtype ? subtype->code : "";


        //externalRef ; ownerName ; dueDate ; productTypeId ; productSubtypeId ; side ; materialBarcode ; color
        QString line = QString("%1;%2;%3;%4;%5;%6;%7;%8;%9\n")
                           .arg(it.key())
                           .arg(ctx.ownerName)
                           .arg(ctx.dueDate.toString("yyyy-MM-dd"))
                           .arg(typeCode)
                           .arg(subtypeCode)
                           .arg(HandlerSideUtils::toDisplayText(ctx.side))
                           .arg(matBarcode)
                           .arg(ctx.color).arg(ctx.surfaceCode);


        f.write(line.toUtf8());
    }
}

void AddInputDialog::updateContextModeLabel()
{
    QString text;

    switch (_contextMode) {
    case ContextMode::Editing:
        text = "Mód: SZERKESZTÉS";
        break;

    case ContextMode::Existing:
        text = "Mód: LÉTEZŐ TÉTELSZÁM";
        break;
    case ContextMode::Sequential:
        text = "Mód: SOROZAT";
        break;
    case ContextMode::NewOrder:
        text = "Mód: ÚJ MEGRENDELŐ";
        break;
    case ContextMode::Update:
        text = "Mód: MÓDOSÍTÁS";
        break;
    }

    QString a;
    switch(_mode){
    case DialogMode::Create:
        a = "Create";
        break;
    case DialogMode::Update:
        a = "Update";
        break;
    }

    ui->label_ContextMode->setText(a+", "+text);
}


void AddInputDialog::applyOwnerAndDate(const RequestContext& ctx)
{
    ui->editOwner->setText(ctx.ownerName);
    ui->editDueDate->setDate(ctx.dueDate);
}

void AddInputDialog::applyProductTypeFromContext(const RequestContext& ctx)
{
    setSelectedProductTypeId(ctx.productTypeId);
}

void AddInputDialog::applyProductSubtypeFromContext(const RequestContext& ctx)
{
    setSelectedProductSubtypeId(ctx.productSubtypeId);
}


void AddInputDialog::applySideFromContext(const RequestContext& ctx)
{
    applySide(ctx.side);
}

void AddInputDialog::applyMaterialFromContext(const RequestContext& ctx)
{
    if (!ctx.defaultMaterialId.isNull()) {
        int idx = ui->comboMaterial->findData(ctx.defaultMaterialId);
        if (idx >= 0)
            ui->comboMaterial->setCurrentIndex(idx);
    }
}

void AddInputDialog::applyColorFromContext(const RequestContext& ctx)
{
    ui->edit_Color->setText(ctx.color);
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
    ui->edit_Color->setText(req.color);
}

void AddInputDialog::applySideFromRequest(const Cutting::Plan::Request& req)
{
    applySide(req.leftCount > 0 ? HandlerSide::Left : HandlerSide::Right);
}
void AddInputDialog::applyMaterialFromRequest(const Cutting::Plan::Request& req)
{
    int idx = ui->comboMaterial->findData(req.materialId);
    if (idx >= 0)
        ui->comboMaterial->setCurrentIndex(idx);
}
void AddInputDialog::applyLengthAndQuantityFromRequest(const Cutting::Plan::Request& req)
{
    ui->editLength->setText(QString::number(req.requiredLength));
    ui->spinQuantity->setValue(req.quantity);

    onQuantityChanged(req.quantity);

    if (req.quantity == 1)
        ui->radioLeft->setChecked(req.leftCount == 1);
    else
        ui->sliderHandler->setValue(req.leftCount);
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
    for (auto* rb : ui->groupBox_productType->findChildren<QRadioButton*>())
        rb->setEnabled(editable);
}
void AddInputDialog::setProductSubtypeEditable(bool editable)
{
    auto* stack = ui->stackedWidget_stackSubtype;
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

void AddInputDialog::applySurfaceFromContext(const RequestContext& ctx)
{
    int idx = ui->comboBox_Surface->findData(ctx.surfaceCode);
    if (idx >= 0)
        ui->comboBox_Surface->setCurrentIndex(idx);
}

void AddInputDialog::applySurfaceFromRequest(const Cutting::Plan::Request& req)
{
    QString code = SurfaceTypeUtils::toCode(req.surface);
    int idx = ui->comboBox_Surface->findData(code);
    if (idx >= 0)
        ui->comboBox_Surface->setCurrentIndex(idx);
}

