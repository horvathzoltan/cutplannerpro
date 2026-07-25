#pragma once

#include <QToolButton>
#include <QMenu>
#include <QComboBox>
#include <QUuid>

class RecentMaterialsWidget : public QToolButton
{
    Q_OBJECT

public:
    explicit RecentMaterialsWidget(QWidget* parent = nullptr);

    // Új anyag megjegyzése (max 5 elem)
    void rememberMaterial(const QUuid& id);

    // Popup menü újraépítése a combo alapján
    void rebuildMenu(QComboBox* combo);

private:
    QMenu* m_menu;

    // Globális recent lista
    static QList<QUuid> s_recent;
};

