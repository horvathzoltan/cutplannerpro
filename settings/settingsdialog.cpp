#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settingsmanager.h"
#include "settingsmeta.h"
#include "settingsmeta.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    buildTabs();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::buildTabs()
{
    // kategóriák → tabok
    QMap<SettingCategory, QString> tabNames = {
        { SettingCategory::General,        "Általános" },
        { SettingCategory::Cutting,        "Vágás" },
        { SettingCategory::MaterialFinder, "Anyagkereső" },
        { SettingCategory::Advanced,       "Haladó" }
    };

    for (auto it = tabNames.begin(); it != tabNames.end(); ++it) {
        QWidget* tab = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(tab);

        buildCategoryTab(it.key(), tab);

        layout->addStretch();
        ui->tabWidget->addTab(tab, it.value());
    }
}
void SettingsDialog::buildCategoryTab(SettingCategory category, QWidget* tab)
{
    QVBoxLayout* layout = static_cast<QVBoxLayout*>(tab->layout());

    for (const auto& meta : SettingsMeta) {

        if (meta.category != category)
            continue;

        QWidget* editor = nullptr;

        switch (meta.type) {

        case SettingType::Bool: {
            auto* chk = new QCheckBox(meta.label);
            chk->setChecked(SettingsManager::instance().value(meta.key, meta.defaultValue).toBool());
            editor = chk;
            break;
        }

        case SettingType::Int: {
            auto* spin = new QSpinBox();
            spin->setRange(0, 100000);
            spin->setValue(SettingsManager::instance().value(meta.key, meta.defaultValue).toInt());
            auto* lbl = new QLabel(meta.label);
            auto* row = new QHBoxLayout();
            row->addWidget(lbl);
            row->addWidget(spin);
            auto* container = new QWidget();
            container->setLayout(row);
            editor = spin;
            layout->addWidget(container);
            editors[meta.key] = spin;
            continue;
        }

        case SettingType::Enum: {
            auto* combo = new QComboBox();
            combo->addItems(meta.enumValues);
            combo->setCurrentText(SettingsManager::instance().value(meta.key, meta.defaultValue).toString());
            auto* lbl = new QLabel(meta.label);
            auto* row = new QHBoxLayout();
            row->addWidget(lbl);
            row->addWidget(combo);
            auto* container = new QWidget();
            container->setLayout(row);
            editor = combo;
            layout->addWidget(container);
            editors[meta.key] = combo;
            continue;
        }

        case SettingType::Action: {
            auto* btn = new QPushButton(meta.label);
            layout->addWidget(btn);

            connect(btn, &QPushButton::clicked, this, [this, meta]() {
                handleAction(meta.key);
            });

            continue;
        }

        case SettingType::Separator: {
            auto* sep = new QFrame();
            sep->setFrameShape(QFrame::HLine);
            layout->addWidget(sep);
            continue;
        }
        }

        layout->addWidget(editor);
        editors[meta.key] = editor;
    }
}

void SettingsDialog::accept()
{
    for (const auto& meta : SettingsMeta) {

        if (!editors.contains(meta.key))
            continue;

        QWidget* w = editors[meta.key];

        switch (meta.type) {

        case SettingType::Bool:
            SettingsManager::instance().setValue(meta.key,
                                                 static_cast<QCheckBox*>(w)->isChecked());
            break;

        case SettingType::Int:
            SettingsManager::instance().setValue(meta.key,
                                                 static_cast<QSpinBox*>(w)->value());
            break;

        case SettingType::Enum:
            SettingsManager::instance().setValue(meta.key,
                                                 static_cast<QComboBox*>(w)->currentText());
            break;

        default:
            break;
        }
    }

    QDialog::accept();
}

void SettingsDialog::reject()
{
    QDialog::reject();
}

void SettingsDialog::handleAction(const QString& key)
{
    if (key == "reset_headers") {
        resetHeaders();
    }
    else if (key == "reset_window") {
        resetWindow();
    }
    else if (key == "reset_counters") {
        resetCounters();
    }
}

void SettingsDialog::resetHeaders()
{
    auto& sm = SettingsManager::instance();

    sm.setValue(SettingsKeys::TableInputHeader, QByteArray());
    sm.setValue(SettingsKeys::TableResultsHeader, QByteArray());
    sm.setValue(SettingsKeys::TableStockHeader, QByteArray());
    sm.setValue(SettingsKeys::TableLeftoversHeader, QByteArray());
    sm.setValue(SettingsKeys::TableStorageAuditHeader, QByteArray());
    sm.setValue(SettingsKeys::TableRelocationOrderHeader, QByteArray());
    sm.setValue(SettingsKeys::CuttingInstructionTableHeader, QByteArray());

    sm.save();
}


void SettingsDialog::resetWindow()
{
    auto& sm = SettingsManager::instance();

    sm.setValue(SettingsKeys::WindowGeometry, QByteArray());
    sm.setValue(SettingsKeys::MainSplitterState, QByteArray());

    sm.save();
}

void SettingsDialog::resetCounters()
{
    auto& sm = SettingsManager::instance();

    sm.setValue(SettingsKeys::MaterialCounter, 0);
    sm.setValue(SettingsKeys::LeftoverCounter, 0);
    sm.setValue(SettingsKeys::ManualLeftoverCounter, 0);

    sm.save();
}


