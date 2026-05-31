#include "clonerequestdialog.h"
#include "ui_clonerequestdialog.h"
#include "materials/registry/material_registry.h"

#include <QHBoxLayout>
#include <QMessageBox>

#include <model/registries/cuttingplanrequestregistry.h>

CloneRequestDialog::CloneRequestDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::CloneRequestDialog)
{
    ui->setupUi(this);

    // perMaterialLayout margók egyszer, a loop előtt
    ui->perMaterialLayout->setContentsMargins(4, 4, 4, 4);
    ui->perMaterialLayout->setSpacing(6);

    ui->scrollAreaWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    ui->scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->perMaterialLayout->addStretch(0); // 0 = ne legyen stretch
    ui->scrollArea->setAlignment(Qt::AlignTop);

    // 1) Plan összes requestjének beolvasása
    auto reqs = CuttingPlanRequestRegistry::instance().readAll();

    // 2) Egyedi material ID-k kigyűjtése (QSet)
    QSet<QUuid> materialIdsSet;
    for (const auto& r : reqs)
        materialIdsSet.insert(r.materialId);

    // Átalakítjuk vektorrá, hogy indexelhető legyen
    QVector<QUuid> materialIds = materialIdsSet.values();

    // 3) Material lista a registryből
    auto allMaterials = MaterialRegistry::instance().readAll();

    // 4) Minden materialhoz létrehozunk egy szép, tagolt blokkot
    for (int i = 0; i < materialIds.size(); ++i) {

        QUuid mid = materialIds[i];

        const MaterialMaster* mat = MaterialRegistry::instance().findById(mid);
        QString matName = mat ? mat->name : "(ismeretlen anyag)";

        //
        // 1) Material név – külön sorban, teljes szélességben
        //
        auto* nameLabel = new QLabel(matName, ui->scrollAreaWidget);
        nameLabel->setStyleSheet("font-weight: bold;");
        ui->perMaterialLayout->addWidget(nameLabel);

        //
        // 2) Combo + delta egy sorban
        //
        auto* row = new QWidget(ui->scrollAreaWidget);
        auto* h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(12);

        auto* combo = new QComboBox(row);
        combo->setMinimumWidth(220);
        combo->addItem("(nincs változás)", QUuid());
        for (const auto& m : allMaterials)
            combo->addItem(m.toDisplay(), m.id);

        auto* spin = new QSpinBox(row);
        spin->setMinimumWidth(70);
        spin->setMaximumWidth(70);
        spin->setMinimum(-1000);
        spin->setMaximum(1000);

        h->addWidget(combo);
        h->addWidget(spin);
        h->addStretch();

        ui->perMaterialLayout->addWidget(row);

        //
        // 3) Separator – csak ha nem az utolsó material
        //
        if (i < materialIds.size() - 1) {
            auto* sep = new QFrame(ui->scrollAreaWidget);
            sep->setFrameShape(QFrame::HLine);
            sep->setFrameShadow(QFrame::Sunken);
            ui->perMaterialLayout->addWidget(sep);
        }

        //
        // 4) Eltároljuk a widgeteket
        //
        MaterialRuleWidgets w;
        w.originalMaterialId = mid;
        w.label = nameLabel;
        w.combo = combo;
        w.spin = spin;
        _materialRules.append(w);
    }
}


CloneRequestDialog::~CloneRequestDialog() {
    delete ui;
}

QVector<CloneMaterialRule> CloneRequestDialog::result() const
{
    QVector<CloneMaterialRule> rules;

    for (const auto& w : _materialRules) {
        CloneMaterialRule r;
        r.originalMaterialId = w.originalMaterialId;
        r.newMaterialId = w.combo->currentData().toUuid();  // null = no change
        r.delta = w.spin->value();                          // 0 = no change
        rules.append(r);
    }

    return rules;
}

QString CloneRequestDialog::tag() const {
    return ui->editTag->text().trimmed();
}


bool CloneRequestDialog::validateInputs()
{
    //
    // 1) TAG validálás
    //
    QString t = ui->editTag->text().trimmed();

    if (t.isEmpty()) {
        QMessageBox::warning(this, "Hiba", "A fájlnév tag nem lehet üres.");
        return false;
    }

    if (t.length() > 32) {
        QMessageBox::warning(this, "Hiba", "A fájlnév tag túl hosszú (max 32 karakter).");
        return false;
    }

    // Multiplatform tiltott karakterek + saját tiltás
    static const QString forbidden = R"(/\:*?"<>|_)";
    for (QChar c : t) {
        if (forbidden.contains(c)) {
            QMessageBox::warning(this, "Hiba",
                                 QString("A fájlnév tag nem tartalmazhatja ezt a karaktert: '%1'").arg(c));
            return false;
        }
    }

    //
    // 2) Material‑wise validálás
    //
    bool anyChange = false;

    for (const auto& w : _materialRules) {

        QUuid newId = w.combo->currentData().toUuid();
        int delta = w.spin->value();

        // Ha bármelyik sorban van változás → OK
        if (!newId.isNull() || delta != 0)
            anyChange = true;

        // ±100 mm limit
        if (delta < -100 || delta > 100) {
            QMessageBox::warning(this, "Hiba",
                                 QString("A hossz módosítás túl nagy: %1 mm (max ±100 mm)").arg(delta));
            return false;
        }
    }

    if (!anyChange) {
        QMessageBox::warning(this, "Hiba",
                             "Legalább egy anyagnál változtatni kell (új anyag vagy hossz módosítás).");
        return false;
    }

    return true;
}

void CloneRequestDialog::accept()
{
    if (!validateInputs())
        return;

    QDialog::accept();
}
