#include "materialsearchdialog.h"
#include "materials/registry/material_registry.h"
#include "view/common/layouts/qflowlayout.h"
#include "view/dialog/materialfinder/materialdelegate.h"

#include <QLabel>

#include <product/registry/material_role_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

static void clearLayout(QLayout* layout)
{
    if (!layout)
        return;

    while (QLayoutItem* item = layout->takeAt(0)) {

        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }

        // Ha layout van benne, NEM töröljük!
        // Csak eltávolítjuk, és Qt majd felszabadítja,
        // amikor a parent panel új layoutot kap.
        if (QLayout* child = item->layout()) {
            clearLayout(child);
        }

        delete item;   // ez már biztonságos
    }
}



MaterialSearchDialog::MaterialSearchDialog(
    QWidget* parent,
    const QString& initialColor,
    const QString& initialType,
    const QString& initialSubtype,
    const QString& initialSearch)
    : QDialog(parent),
    model(new QStandardItemModel(this)),
    colorButtons(new QButtonGroup(this))
{
    initColor = initialColor;
    initType = initialType;
    initSubtype = initialSubtype;
    initSearch = initialSearch;


    setWindowTitle("Anyag keresése");
    resize(600, 500);

    auto* layout = new QVBoxLayout(this);

    // 1) SZÍN SZŰRŐ PANEL
    colorFilterPanel = new QWidget(this);
    auto* colorLayout = new QHBoxLayout(colorFilterPanel);
    colorLayout->setContentsMargins(0,0,0,0);
    layout->addWidget(colorFilterPanel);

    categoryFilterPanel = new QWidget(this);
    auto* catLayout = new QVBoxLayout(categoryFilterPanel);
    categoryFilterPanel->setLayout(catLayout);
    layout->addWidget(categoryFilterPanel);

    typePanel = new QWidget(this);
    typeLayout = new QVBoxLayout(typePanel);
    typePanel->setLayout(typeLayout);
    catLayout->addWidget(typePanel);

    subtypePanel = new QWidget(this);
    subtypeLayout = new QVBoxLayout(subtypePanel);
    subtypePanel->setLayout(subtypeLayout);
    catLayout->addWidget(subtypePanel);

    buildColorButtons();


    // ⭐ ProductType gombok
    typeButtons = new QButtonGroup(this);
    buildTypeButtons();


    // ⭐ ProductSubtype gombok
    subtypeButtons = new QButtonGroup(this);
    buildSubtypeButtons();

    connect(typeButtons, &QButtonGroup::idClicked, this, [this]() {
        QTimer::singleShot(0, this, [this]() {
            buildSubtypeButtons();
            applyFilter(searchEdit->text());
        });
    });

    connect(subtypeButtons, &QButtonGroup::idClicked, this, [this]() {
        applyFilter(searchEdit->text());
    });


    // ⭐ előválasztás: szín
    for (auto* btn : colorButtons->buttons()) {
        if (btn->property("colorCode").toString() == initColor) {
            btn->setChecked(true);
            break;
        }
    }


    // ⭐ előválasztás: type
    for (auto* btn : typeButtons->buttons()) {
        if (btn->property("typeCode").toString() == initType)
            btn->setChecked(true);
    }

    // ⭐ subtype gombsor újraépítése a type alapján
    buildSubtypeButtons();

    // ⭐ előválasztás: subtype
    for (auto* btn : subtypeButtons->buttons()) {
        if (btn->property("subtypeCode").toString() == initSubtype)
            btn->setChecked(true);
    }

    applyFilter(initSearch);


    // 2) KERESŐMEZŐ
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Írj be legalább 3 karaktert (név, barcode, external code)...");
    layout->addWidget(searchEdit);

    if (!initSearch.isEmpty())
        searchEdit->setText(initSearch);


    // 3) TALÁLATI LISTA
    resultList = new QListView(this);
    resultList->setModel(model);
    resultList->setItemDelegate(new MaterialDelegate(this));
    layout->addWidget(resultList);

    // 4) GOMBOK
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // ANYAGOK BETÖLTÉSE
    allMaterials = MaterialRegistry::instance().readAll();

    // DEBOUNCE
    debounce.setInterval(250);
    debounce.setSingleShot(true);
    connect(&debounce, &QTimer::timeout, this, [this]() {
        applyFilter(searchEdit->text());
    });

    connect(searchEdit, &QLineEdit::textChanged, this, [this]() {
        debounce.start();
    });

    connect(colorButtons, &QButtonGroup::idClicked, this, [this]() {
        applyFilter(searchEdit->text());
    });

    // KETTŐS KATTINTÁS → OK
    connect(resultList, &QListView::doubleClicked, this, [this](const QModelIndex& ix) {
        resultList->setCurrentIndex(ix);
        QDialog::accept();
    });

    applyFilter(initSearch);
}

MaterialSearchDialog::~MaterialSearchDialog() {}


// ⭐ SZÍN GOMBOK DINAMIKUS GENERÁLÁSA
void MaterialSearchDialog::buildColorButtons()
{
    auto materials = MaterialRegistry::instance().readAll();
    QSet<QString> colorCodes;   // ⭐ RAL/HEX kódok
    QMap<QString, QString> codeToName; // ⭐ kód → emberi név

    for (const auto& m : materials) {
        QString code = m.color.code();   // pl. "7016"
        QString name = m.color.name();   // pl. "Anthracite Grey"

        if (!code.isEmpty()) {
            colorCodes.insert(code);
            codeToName[code] = name;
        }
    }

    auto* layout = qobject_cast<QHBoxLayout*>(colorFilterPanel->layout());

    // ⭐ "Nincs szín" gomb
    auto* noneBtn = new QRadioButton("Nincs szín");
    noneBtn->setChecked(true);
    noneBtn->setProperty("colorCode", "Nincs");
    colorButtons->addButton(noneBtn, -1);
    layout->addWidget(noneBtn);

    // ⭐ Színek
    int id = 0;
    for (const QString& code : colorCodes) {

        QString display = QString("%1 – %2")
                              .arg(code)
                              .arg(codeToName.value(code));

        auto* btn = new QRadioButton(display);
        btn->setProperty("colorCode", code);   // ⭐ RAL/HEX kód property-ben

        colorButtons->addButton(btn, id++);
        layout->addWidget(btn);
    }

    layout->addStretch();
}



static int levenshtein(const QString& s1, const QString& s2)
{
    const int n = s1.size();
    const int m = s2.size();
    if (n == 0) return m;
    if (m == 0) return n;

    QVector<int> prev(m + 1), curr(m + 1);

    for (int j = 0; j <= m; ++j)
        prev[j] = j;

    for (int i = 1; i <= n; ++i) {
        curr[0] = i;
        for (int j = 1; j <= m; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            curr[j] = std::min({ prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost });
        }
        prev = curr;
    }
    return curr[m];
}

// ⭐ SZŰRÉS (prefix + substring)
void MaterialSearchDialog::applyFilter(const QString& text)
{
    model->clear();

    QString t = text.trimmed().toLower();

    QVector<MaterialMaster> exactMatches;
    QVector<MaterialMaster> prefixMatches;
    QVector<MaterialMaster> substringMatches;
    QVector<MaterialMaster> fuzzyMatches;

    QString typeCode = selectedType();
    QUuid typeId;

    if (typeCode != "Mind") {
        for (const auto& t : ProductTypeRegistry::instance().readAll()) {
            if (t.code == typeCode) {
                typeId = t.id;
                break;
            }
        }
    }

    QString subtypeCode = selectedSubtype();
    QUuid subtypeId;

    if (subtypeCode != "Mind") {
        for (const auto& st : ProductSubtypeRegistry::instance().readAll()) {
            if (st.code == subtypeCode) {
                subtypeId = st.id;
                break;
            }
        }
    }

    QVector<MaterialRole> roles =
        MaterialRoleRegistry::instance().findRoles(typeId, subtypeId);

    QSet<MaterialFamily> allowedFamilies;
    QStringList allowedPrefixes;

    for (const auto& r : roles) {
        allowedFamilies.insert(r.family);
        allowedPrefixes.append(r.barcodePrefix);
    }



    bool useRoleFilter =
        (typeCode != "Mind") &&
        (subtypeCode != "Mind");


    // Ha nincs keresőkifejezés → teljes lista (szín szerint)
    if (t.length() < 3) {
        for (const auto& m : allMaterials) {

            // SZÍN SZŰRÉS
            QString selectedCode = selectedColorCode();  // új függvény
            if (selectedCode != "Nincs" && m.color.code() != selectedCode)
                continue;


            if (useRoleFilter) {
                // Family szűrés
                if (!allowedFamilies.contains(m.family))
                    continue;

                // Barcode prefix szűrés
                bool prefixOk = false;
                for (const QString& p : allowedPrefixes) {
                    QString px = p;
                    if (px.endsWith("*"))
                        px.chop(1);

                    if (m.barcode.startsWith(px)) {
                        prefixOk = true;
                        break;
                    }
                }

                if (!prefixOk)
                    continue;
            }

            auto* item = new QStandardItem();
            item->setData(QVariant::fromValue(m), Qt::UserRole);
            item->setData(m.name, Qt::DisplayRole);
            model->appendRow(item);
        }
        return;
    }


    // 4-szintű keresés
    for (const auto& m : allMaterials) {

        QString selectedCode = selectedColorCode();
        if (selectedCode != "Nincs" && m.color.code() != selectedCode)
            continue;


        QString name = m.name.toLower();
        QString bc   = m.barcode.toLower();
        QString ext  = m.externalCode.toLower();

        bool isExact =
            (name == t) ||
            (bc == t) ||
            (ext == t);

        bool isPrefix =
            name.startsWith(t) ||
            bc.startsWith(t) ||
            ext.startsWith(t);

        bool isSubstring =
            name.contains(t) ||
            bc.contains(t) ||
            ext.contains(t);

        int d1 = levenshtein(name, t);
        int d2 = levenshtein(bc, t);
        int d3 = levenshtein(ext, t);
        bool isFuzzy = (d1 <= 1 || d2 <= 1 || d3 <= 1);

        if (isExact)
            exactMatches.append(m);
        else if (isPrefix)
            prefixMatches.append(m);
        else if (isSubstring)
            substringMatches.append(m);
        else if (isFuzzy)
            fuzzyMatches.append(m);
    }

    // 1) Exact match
    if (!exactMatches.isEmpty()) {
        addSeparator("Pontos egyezés");
        for (const auto& m : exactMatches) {
            auto* item = new QStandardItem();
            item->setData(QVariant::fromValue(m), Qt::UserRole);
            item->setData(m.name, Qt::DisplayRole);
            model->appendRow(item);
        }
    }

    // 2) Prefix match
    if (!prefixMatches.isEmpty()) {
        addSeparator("Kezdődik ezzel");
        for (const auto& m : prefixMatches) {
            auto* item = new QStandardItem();
            item->setData(QVariant::fromValue(m), Qt::UserRole);
            item->setData(m.name, Qt::DisplayRole);
            model->appendRow(item);
        }
    }

    // 3) Substring match
    if (!substringMatches.isEmpty()) {
        addSeparator("Tartalmazza");
        for (const auto& m : substringMatches) {
            auto* item = new QStandardItem();
            item->setData(QVariant::fromValue(m), Qt::UserRole);
            item->setData(m.name, Qt::DisplayRole);
            model->appendRow(item);
        }
    }

    // 4) Fuzzy match
    if (!fuzzyMatches.isEmpty()) {
        addSeparator("Hasonló (elgépelés)");
        for (const auto& m : fuzzyMatches) {
            auto* item = new QStandardItem();
            item->setData(QVariant::fromValue(m), Qt::UserRole);
            item->setData(m.name, Qt::DisplayRole);
            model->appendRow(item);
        }
    }

    // Ha 1 találat → automatikus kijelölés
    if (model->rowCount() == 1)
        resultList->setCurrentIndex(model->index(0,0));
}





// ⭐ KIVÁLASZTÁS VISSZAADÁSA
MaterialSelection MaterialSearchDialog::selection() const
{
    MaterialSelection s;

    QModelIndex ix = resultList->currentIndex();
    if (!ix.isValid())
        return s;

    MaterialMaster m =
        ix.data(Qt::UserRole).value<MaterialMaster>();

    s.id = m.id;
    s.master = m;
    return s;
}


// ⭐ AKTUÁLIS SZÍN LEKÉRÉSE
// QString MaterialSearchDialog::selectedColor() const
// {
//     QAbstractButton* btn = colorButtons->checkedButton();
//     if (!btn)
//         return "Nincs";

//     return btn->text();
// }

void MaterialSearchDialog::addSeparator(const QString& title)
{
    auto* sep = new QStandardItem("── " + title + " ──");
    sep->setFlags(Qt::NoItemFlags);
    sep->setData(true, Qt::UserRole + 1); // jelölés: separator
    model->appendRow(sep);
}


void MaterialSearchDialog::buildTypeButtons()
{
    // panel + gyerek layoutok teljes kiürítése
    clearLayout(typeLayout);

    // gombcsoport ürítése
    for (auto* btn : typeButtons->buttons())
        typeButtons->removeButton(btn);


    // szeparátor
    auto* sep = new QLabel("Típusok");
    sep->setStyleSheet("font-weight: bold; margin-bottom: 4px;");
    typeLayout->addWidget(sep);

    // gombsor
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 6);
    row->setAlignment(Qt::AlignLeft);

    typeLayout->addLayout(row);

    // "Mind" gomb
    auto* allBtn = new QRadioButton("Mind");
    allBtn->setChecked(true);
    allBtn->setProperty("typeCode", "Mind");
    typeButtons->addButton(allBtn);
    row->addWidget(allBtn);

    // típus gombok
    for (const auto& t : ProductTypeRegistry::instance().readAll()) {
        auto* btn = new QRadioButton(t.name);
        btn->setProperty("typeCode", t.code);
        typeButtons->addButton(btn);
        row->addWidget(btn);
    }


    // connect(typeButtons, &QButtonGroup::idClicked, this, [this]() {
    //     // subtype reset
    //     // ⭐ subtype reset minden esetben
    //     for (auto* btn : subtypeButtons->buttons())
    //         btn->setChecked(false);


    //     // ⭐ subtype újraépítése garantáltan a következő event loop ciklusban
    //     QTimer::singleShot(0, this, [this]() {
    //         buildSubtypeButtons();
    //         applyFilter(searchEdit->text());
    //     });

    // });

}

void MaterialSearchDialog::buildSubtypeButtons()
{
    // panel + gyerek layoutok teljes kiürítése
    clearLayout(subtypeLayout);

    // gombcsoport ürítése
    for (auto* btn : subtypeButtons->buttons())
        subtypeButtons->removeButton(btn);


    // szeparátor
    auto* sep = new QLabel("Altípusok");
    sep->setStyleSheet("font-weight: bold; margin-top: 6px;");
    subtypeLayout->addWidget(sep);

    // gombsor
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 6, 0, 0);
    row->setAlignment(Qt::AlignLeft);
    subtypeLayout->addLayout(row);

    // "Mind" gomb
    auto* allBtn = new QRadioButton("Mind");
    allBtn->setChecked(true);
    allBtn->setProperty("subtypeCode", "Mind");
    subtypeButtons->addButton(allBtn);
    row->addWidget(allBtn);

    // typeId lekérése
    QString typeCode = selectedType();
    bool typeIsMind = (typeCode == "Mind");

    if (typeIsMind) {
        // csak a Mind gomb maradjon
        return;
    }

    QUuid typeId;

    if (typeCode != "Mind") {
        for (const auto& t : ProductTypeRegistry::instance().readAll()) {
            if (t.code == typeCode) {
                typeId = t.id;
                break;
            }
        }
    }

    // altípus gombok
    for (const auto& st : ProductSubtypeRegistry::instance().readAll()) {
        if (typeCode != "Mind" && st.typeId != typeId)
            continue;

        auto* btn = new QRadioButton(st.name);
        btn->setProperty("subtypeCode", st.code);
        subtypeButtons->addButton(btn);
        row->addWidget(btn);
    }

    // reset + Mind kiválasztása
    for (auto* btn : subtypeButtons->buttons())
        btn->setChecked(false);

    if (!subtypeButtons->buttons().isEmpty())
        subtypeButtons->buttons().first()->setChecked(true);

    // connect(subtypeButtons, &QButtonGroup::idClicked, this, [this]() {
    //     applyFilter(searchEdit->text());
    // });

}


QString MaterialSearchDialog::selectedSubtype() const
{
    QAbstractButton* btn = subtypeButtons->checkedButton();
    if (!btn)
        return "Mind";
    return btn->property("subtypeCode").toString();
}


QString MaterialSearchDialog::selectedType() const
{
    QAbstractButton* btn = typeButtons->checkedButton();
    if (!btn)
        return "Mind";
    return btn->property("typeCode").toString();
}

QString MaterialSearchDialog::selectedColorCode() const
{
    QAbstractButton* btn = colorButtons->checkedButton();
    if (!btn)
        return "Nincs";

    return btn->property("colorCode").toString();
}

