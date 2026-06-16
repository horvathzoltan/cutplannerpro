#include "materialsearchdialog.h"
#include "materials/registry/material_registry.h"
#include "view/common/layouts/qflowlayout.h"
#include "view/dialog/materialfinder/materialdelegate.h"

MaterialSearchDialog::MaterialSearchDialog(QWidget* parent)
    : QDialog(parent),
    model(new QStandardItemModel(this)),
    colorButtons(new QButtonGroup(this))
{
    setWindowTitle("Anyag keresése");
    resize(600, 500);

    auto* layout = new QVBoxLayout(this);

    // 1) SZÍN SZŰRŐ PANEL
    colorFilterPanel = new QWidget(this);
    auto* colorLayout = new QHBoxLayout(colorFilterPanel);
    colorLayout->setContentsMargins(0,0,0,0);
    layout->addWidget(colorFilterPanel);

    buildColorButtons();


    // 1/b) KATEGÓRIA SZŰRŐ PANEL
    categoryFilterPanel = new QWidget(this);
    //auto* catLayout = new QHBoxLayout(categoryFilterPanel);
    auto* catLayout = new QFlowLayout(categoryFilterPanel);
    //catLayout->setContentsMargins(0,0,0,0);
    categoryFilterPanel->setLayout(catLayout);
    layout->addWidget(categoryFilterPanel);

    categoryButtons = new QButtonGroup(this);

    // 2) KERESŐMEZŐ
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Írj be legalább 3 karaktert (név, barcode, external code)...");
    layout->addWidget(searchEdit);

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

    buildCategoryButtons();

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

    applyFilter("");
}

MaterialSearchDialog::~MaterialSearchDialog() {}


// ⭐ SZÍN GOMBOK DINAMIKUS GENERÁLÁSA
void MaterialSearchDialog::buildColorButtons()
{
    auto materials = MaterialRegistry::instance().readAll();
    QSet<QString> colors;

    for (const auto& m : materials)
        colors.insert(m.color.name());

    auto* layout = qobject_cast<QHBoxLayout*>(colorFilterPanel->layout());

    // "Nincs" gomb
    auto* noneBtn = new QRadioButton("Nincs");
    noneBtn->setChecked(true);
    colorButtons->addButton(noneBtn, -1);
    layout->addWidget(noneBtn);

    // Színek
    int id = 0;
    for (const QString& c : colors) {
        auto* btn = new QRadioButton(c);
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

    QString color = selectedColor();
    QString t = text.trimmed().toLower();
    QString cat = selectedCategory();

    QVector<MaterialMaster> exactMatches;
    QVector<MaterialMaster> prefixMatches;
    QVector<MaterialMaster> substringMatches;
    QVector<MaterialMaster> fuzzyMatches;

    // Ha nincs keresőkifejezés → teljes lista (szín szerint)
    if (t.length() < 3) {
        for (const auto& m : allMaterials) {

            // SZÍN SZŰRÉS
            if (color != "Nincs" && m.color.name() != color)
                continue;

            // KATEGÓRIA SZŰRÉS
            QStringList toks = extractTokens(m.name);
            if (cat != "Mind" && !toks.contains(cat))
                continue;

            auto* item = new QStandardItem();
            item->setData(QVariant::fromValue(m), Qt::UserRole);
            item->setData(m.name, Qt::DisplayRole);
            model->appendRow(item);
        }
        return;
    }


    // 4-szintű keresés
    for (const auto& m : allMaterials) {

        if (color != "Nincs" && m.color.name() != color)
            continue;

        // Kategória szűrés
        QStringList toks = extractTokens(m.name);
        if (cat != "Mind" && !toks.contains(cat))
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
QString MaterialSearchDialog::selectedColor() const
{
    QAbstractButton* btn = colorButtons->checkedButton();
    if (!btn)
        return "Nincs";

    return btn->text();
}

void MaterialSearchDialog::addSeparator(const QString& title)
{
    auto* sep = new QStandardItem("── " + title + " ──");
    sep->setFlags(Qt::NoItemFlags);
    sep->setData(true, Qt::UserRole + 1); // jelölés: separator
    model->appendRow(sep);
}


QString MaterialSearchDialog::prefixOf(const QString& name) const
{
    QStringList parts = name.split(" ", Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return "Egyéb";

    QString p = parts[0].trimmed();

    // RAL kódok külön kategória
    if (p.startsWith("RAL", Qt::CaseInsensitive))
        return "RAL";

    return p;
}

// void MaterialSearchDialog::buildCategoryButtons()
// {
//     QSet<QString> cats;

//     for (const auto& m : allMaterials)
//         cats.insert(prefixOf(m.name));

//     auto* layout = qobject_cast<QHBoxLayout*>(categoryFilterPanel->layout());

//     // "Mind" gomb
//     auto* allBtn = new QRadioButton("Mind");
//     allBtn->setChecked(true);
//     categoryButtons->addButton(allBtn, -1);
//     layout->addWidget(allBtn);

//     int id = 0;
//     for (const QString& c : cats) {
//         auto* btn = new QRadioButton(c);
//         categoryButtons->addButton(btn, id++);
//         layout->addWidget(btn);
//     }

//     layout->addStretch();

//     connect(categoryButtons, &QButtonGroup::idClicked, this, [this]() {
//         applyFilter(searchEdit->text());
//     });
// }

QString MaterialSearchDialog::selectedCategory() const
{
    QAbstractButton* btn = categoryButtons->checkedButton();
    if (!btn)
        return "Mind";

    return btn->text();
}

QStringList MaterialSearchDialog::extractTokens(const QString& name) const
{
    QString n = name;

    // 1) CamelCase szétszedése (ha van)
    n.replace(QRegularExpression("([a-záéíóöőúüű])([A-ZÁÉÍÓÖŐÚÜŰ])"), "\\1 \\2");

    // 2) számok, kötőjelek, speciális karakterek → szóköz
    n.replace(QRegularExpression("[^A-Za-zÁÉÍÓÖŐÚÜŰáéíóöőúüű]"), " ");

    // 3) tokenizálás
    QStringList parts = n.split(" ", Qt::SkipEmptyParts);

    // 4) kiszűrjük a túl rövid vagy értelmetlen tokeneket
    QStringList result;
    for (QString p : parts) {
        if (p.length() < 3) continue;        // pl. "Ø18", "NP"
        if (p.toLower() == "mm") continue;   // mértékegység
        result << p;
    }

    if (result.isEmpty())
        result << "Egyéb";

    return result;
}

void MaterialSearchDialog::buildCategoryButtons()
{
    QMap<QString,int> freq;

    // 1) tokenek összegyűjtése és gyakoriság számítása
    for (const auto& m : allMaterials) {
        for (const QString& t : extractTokens(m.name))
            freq[t]++;
    }

    // 2) csak a gyakori tokenekből lesz kategória
    QList<QString> cats;
    for (auto it = freq.begin(); it != freq.end(); ++it) {
        if (it.value() >= 3)   // legalább 3 anyagban szerepel
            cats << it.key();
    }

    cats.sort(Qt::CaseInsensitive);

    auto* layout = static_cast<QFlowLayout*>(categoryFilterPanel->layout());


    // "Mind" gomb
    auto* allBtn = new QRadioButton("Mind");
    allBtn->setChecked(true);
    categoryButtons->addButton(allBtn, -1);
    layout->addWidget(allBtn);

    int id = 0;
    for (const QString& c : cats) {
        auto* btn = new QRadioButton(c);
        categoryButtons->addButton(btn, id++);
        layout->addWidget(btn);
    }

    //layout->addStretch();

    connect(categoryButtons, &QButtonGroup::idClicked, this, [this]() {
        applyFilter(searchEdit->text());
    });
}

