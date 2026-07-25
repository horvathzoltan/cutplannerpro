#include "recentmaterialswidget.h"

QList<QUuid> RecentMaterialsWidget::s_recent;

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
    s_recent.removeAll(id);
    s_recent.prepend(id);

    if (s_recent.size() > 5)
        s_recent.removeLast();
}

void RecentMaterialsWidget::rebuildMenu(QComboBox* combo)
{
    m_menu->clear();

    for (const QUuid& id : s_recent) {
        int idx = combo->findData(id);
        if (idx >= 0) {
            QAction* act = m_menu->addAction(combo->itemText(idx));
            act->setData(id);
        }
    }

    connect(m_menu, &QMenu::triggered, this, [combo](QAction* act){
        QUuid id = act->data().toUuid();
        int idx = combo->findData(id);
        if (idx >= 0)
            combo->setCurrentIndex(idx);
    });
}
