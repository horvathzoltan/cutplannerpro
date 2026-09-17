#include "materialsearchdialog.h"
#include "common/stringsimilarity_helper.h"
#include "materials/registry/material_registry.h"
#include "view/common/layouts/qflowlayout.h"
#include "view/dialog/materialfinder/materialdelegate.h"

#include <QLabel>

#include <product/registry/material_role_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <materials/model/material_family_utils.h>

#include <materials/registry/material_rolegroup_registry.h>
#include <materials/registry/material_storagegroupregistry.h>

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

    // 1) MIND (nincs szűrés)
    auto* allBtn = new QRadioButton("Mind");
    allBtn->setChecked(true);
    allBtn->setProperty("colorCode", "ALL");
    colorButtons->addButton(allBtn, -1);
    layout->addWidget(allBtn);

    // 2) NATÚR (szín nélküli anyagok)
    auto* rawBtn = new QRadioButton("Natúr");
    rawBtn->setProperty("colorCode", "RAW");
    colorButtons->addButton(rawBtn, -2);
    layout->addWidget(rawBtn);

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


// ⭐ SZŰRÉS (prefix + substring)
void MaterialSearchDialog::applyFilter(const QString& text)
{
    model->clear();

    QString t0 = text.trimmed().toLower();

    auto normalize = [](QString s) {
        // 1) lowercase
        s = s.toLower();

        // 2) Unicode canonical decomposition (NFD)
        QString decomposed = s.normalized(QString::NormalizationForm_D);

        // 3) combining diacritics eltávolítása
        QString result;
        for (QChar c : decomposed) {
            if (c.category() != QChar::Mark_NonSpacing &&
                c.category() != QChar::Mark_SpacingCombining &&
                c.category() != QChar::Mark_Enclosing)
            {
                result.append(c);
            }
        }

        return result;
    };

    QString t = normalize(t0);



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

    // ⭐ Role‑lista a Mind logika szerint
    QVector<MaterialRole> roles;
    bool useRoleFilter = false;

    if (typeCode == "Mind") {
        // ⭐ Típus = Mind → nincs role‑szűrés, minden anyag jöhet
        roles.clear();
        useRoleFilter = false;
    } else {
        // ⭐ Van konkrét típus
        if (subtypeCode == "Mind") {
            // ⭐ Altípus = Mind → az adott típus ÖSSZES altípusának role‑jai
            for (const auto& r : MaterialRoleRegistry::instance().readAll()) {
                if (r.productTypeId == typeId) {
                    roles.append(r);
                }
            }
            useRoleFilter = true;
        } else {
            // ⭐ Konkrét típus + konkrét altípus
            roles =
                MaterialRoleRegistry::instance().findRoles(typeId, subtypeId);
            useRoleFilter = true;
        }
    }

    QSet<MaterialFamily> allowedFamilies;
    QSet<QUuid> allowedMaterialIds;

    // ⭐ Role‑okból tárolási csoport → anyag ID‑k
    for (const auto& r : roles)
    {
        allowedFamilies.insert(r.family);

        QUuid sgId = r.storageGroupId;
        if (sgId.isNull())
            continue;

        const MaterialStorageGroup* sg =
            MaterialStorageGroupRegistry::instance().findById(sgId);
        if (!sg)
            continue;

        for (const QUuid& matId : sg->members)
            allowedMaterialIds.insert(matId);
    }



    // Ha nincs keresőkifejezés → teljes lista (szín szerint)
    if (t.length() < 3) {
        for (const auto& m : allMaterials) {

            // SZÍN SZŰRÉS
            QString selectedCode = selectedColorCode();

            if (selectedCode == "ALL") {
                // nincs szűrés
            }
            else if (selectedCode == "RAW") {
                // csak natúr anyagok
                if (m.color.isValid())
                    continue;
            }
            else {
                // konkrét szín
                if (m.color.code() != selectedCode)
                    continue;
            }


            if (useRoleFilter) {
                if (!allowedFamilies.contains(m.family))
                    continue;

                if (!allowedMaterialIds.contains(m.id))
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

        if (selectedCode == "ALL") {
            // nincs szűrés
        }
        else if (selectedCode == "RAW") {
            // natúr anyagok → nincs színkód
            if (m.color.isValid())
                continue;
        }
        else {
            // konkrét színkód
            if (m.color.code() != selectedCode)
                continue;
        }

        // ⭐ TÁROLÁSI SZŰRÉS
        if (useRoleFilter) {
            if (!allowedFamilies.contains(m.family))
                continue;

            if (!allowedMaterialIds.contains(m.id))
                continue;
        }

        // QString name = m.name.toLower();
        // QString bc   = m.barcode.toLower();
        // QString ext  = m.externalCode.toLower();

        // QStringList fields = { name, bc, ext };

        // bool isExact      = StringSimilarity::anyExact(fields, t);
        // bool isPrefix     = StringSimilarity::anyPrefix(fields, t);
        // bool isSubstring  = StringSimilarity::anySubstring(fields, t);
        // bool isFuzzy      = StringSimilarity::anyFuzzy(fields, t);

        // if (isExact)
        //     exactMatches.append(m);
        // else if (isPrefix)
        //     prefixMatches.append(m);
        // else if (isSubstring)
        //     substringMatches.append(m);
        // else if (isFuzzy)
        //     fuzzyMatches.append(m);

        QString name = m.name.toLower();
        QString bc   = m.barcode.toLower();
        QString ext  = m.externalCode.toLower();

        QStringList fields = { name, bc, ext };

        bool isExact = false;
        bool isPrefix = false;
        bool isSubstring = false;

        for (const auto& f0 : fields) {
            QString f = normalize(f0);

            if (f == t) {
                isExact = true;
                break;
            }
            if (f.startsWith(t)) {
                isPrefix = true;
            }
            if (f.contains(t)) {
                isSubstring = true;
            }
        }

        // egyszerű fuzzy: engedjük el, vagy hagyd meg a régi helperrel, ha akarod
        //bool isFuzzy = false;
        // pl. ha akarod:
        bool isFuzzy = StringSimilarity::anyFuzzy(fields, t);

        if (isExact)
            exactMatches.append(m);
        else if (isPrefix)
            prefixMatches.append(m);
        else if (isSubstring)
            substringMatches.append(m);
        else if (isFuzzy)
            fuzzyMatches.append(m);

    }

    // // 1) Exact match
    // if (!exactMatches.isEmpty()) {
    //     addSeparator("Pontos egyezés");
    //     QMap<MaterialFamily, QVector<MaterialMaster>> grouped;

    //     for (const auto& m : exactMatches)
    //         grouped[m.family].append(m);

    //     for (auto it = grouped.begin(); it != grouped.end(); ++it) {
    //         addSeparator(QString("Család: %1").arg(MaterialFamilyUtils::toString(it.key())));

    //         for (const auto& m : it.value()) {
    //             auto* item = new QStandardItem();
    //             item->setData(QVariant::fromValue(m), Qt::UserRole);
    //             item->setData(m.name, Qt::DisplayRole);
    //             model->appendRow(item);
    //         }
    //     }

    // }

    // // 2) Prefix match
    // if (!prefixMatches.isEmpty()) {
    //     addSeparator("Kezdődik ezzel");
    //     for (const auto& m : prefixMatches) {
    //         auto* item = new QStandardItem();
    //         item->setData(QVariant::fromValue(m), Qt::UserRole);
    //         item->setData(m.name, Qt::DisplayRole);
    //         model->appendRow(item);
    //     }
    // }

    // // 3) Substring match
    // if (!substringMatches.isEmpty()) {
    //     addSeparator("Tartalmazza");
    //     for (const auto& m : substringMatches) {
    //         auto* item = new QStandardItem();
    //         item->setData(QVariant::fromValue(m), Qt::UserRole);
    //         item->setData(m.name, Qt::DisplayRole);
    //         model->appendRow(item);
    //     }
    // }

    // // 4) Fuzzy match
    // if (!fuzzyMatches.isEmpty()) {
    //     addSeparator("Hasonló (elgépelés)");
    //     for (const auto& m : fuzzyMatches) {
    //         auto* item = new QStandardItem();
    //         item->setData(QVariant::fromValue(m), Qt::UserRole);
    //         item->setData(m.name, Qt::DisplayRole);
    //         model->appendRow(item);
    //     }
    // }

    //
    // ⭐ RELEVÁNS TALÁLATOK BLOKKJA (felül)
    //
    bool hasRelevant = false;

    auto addRelevantBlock = [&](const QString& title, const QVector<MaterialMaster>& list) {
        if (list.isEmpty())
            return;

        hasRelevant = true;

        addSeparator(title);
        for (const auto& m : list) {
            auto* item = new QStandardItem();
            item->setData(QVariant::fromValue(m), Qt::UserRole);
            item->setData(m.name, Qt::DisplayRole);
            model->appendRow(item);
        }
    };

    // releváns találatok
    addRelevantBlock("Pontos egyezés", exactMatches);
    addRelevantBlock("Kezdődik ezzel", prefixMatches);
    addRelevantBlock("Tartalmazza", substringMatches);
    addRelevantBlock("Hasonló (elgépelés)", fuzzyMatches);


    //
    // ⭐ NEM RELEVÁNS TALÁLATOK BLOKKJA (alul)
    //
    QVector<MaterialMaster> nonRelevant;

    for (const auto& m : allMaterials) {

        // színszűrés
        QString selectedCode = selectedColorCode();
        if (selectedCode == "RAW") {
            if (m.color.isValid())
                continue;
        } else if (selectedCode != "ALL") {
            if (m.color.code() != selectedCode)
                continue;
        }

        // tárolási szűrés
        if (useRoleFilter) {
            if (!allowedFamilies.contains(m.family))
                continue;
            if (!allowedMaterialIds.contains(m.id))
                continue;
        }

        // releváns találatokat kihagyjuk
        auto isRelevant = [&](const MaterialMaster& mm) {
            auto hasId = [&](const QVector<MaterialMaster>& vec) {
                for (const auto& x : vec)
                    if (x.id == mm.id)
                        return true;
                return false;
            };

            return hasId(exactMatches)
                   || hasId(prefixMatches)
                   || hasId(substringMatches)
                   || hasId(fuzzyMatches);
        };

        if (isRelevant(m))
            continue;


        nonRelevant.append(m);
    }

    // ha vannak nem releváns találatok → külön blokk
    if (!nonRelevant.isEmpty()) {
        addSeparator("Egyéb anyagok");
        for (const auto& m : nonRelevant) {
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

