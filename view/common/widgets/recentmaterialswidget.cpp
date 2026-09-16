#include "recentmaterialswidget.h"
#include "common/logger.h"
#include <settings/settingsmanager.h>
#include <materials/registry/material_registry.h>

RecentMaterialsWidget::RecentMaterialsWidget(QWidget* parent)
    : QToolButton(parent),
    m_menu(new QMenu(this))
{
    setText("▼");
    setMenu(m_menu);
    setPopupMode(QToolButton::InstantPopup);

    // 🔥 A kis nyíl eltüntetése
    setStyleSheet("QToolButton::menu-indicator { image: none; }");
}

void RecentMaterialsWidget::rememberMaterial(const QUuid& id)
{
    auto mat = MaterialRegistry::instance().findById(id);
    QString bc = mat->barcode.trimmed();

    if (bc.isEmpty())
        return;

    s_recentBarcodes.removeAll(bc);
    s_recentBarcodes.prepend(bc);

    while (s_recentBarcodes.size() > 5)
        s_recentBarcodes.removeLast();

    savePersistent();   // 🔥 azonnal mentjük
}


void RecentMaterialsWidget::rebuildMenu(QComboBox* combo)
{
    if(_seed.isEmpty()){
        zWarning("RecentMaterialWidget: seed nincs megadva!");
        return;
    }

    m_menu->clear();

    for (int r = 0; r < s_recentBarcodes.size(); ++r) {
        const QString& bc = s_recentBarcodes.at(r);

        // anyag keresése barcode alapján
        for (int i = 0; i < combo->count(); ++i) {
            QUuid id = combo->itemData(i).toUuid();
            auto mat = MaterialRegistry::instance().findById(id);

            if (mat->barcode == bc) {
                QAction* act = m_menu->addAction(combo->itemText(i));
                act->setData(id);
                break;
            }
        }
    }

    connect(m_menu, &QMenu::triggered, this, [combo](QAction* act){
        QUuid id = act->data().toUuid();
        int idx = combo->findData(id);
        if (idx >= 0)
            combo->setCurrentIndex(idx);
    });
}


void RecentMaterialsWidget::loadPersistent()
{
    s_recentBarcodes =
        SettingsManager::instance().recentMaterials(_seed);
}

void RecentMaterialsWidget::savePersistent()
{
    SettingsManager::instance().setRecentMaterials(_seed, s_recentBarcodes);
}
