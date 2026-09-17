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

    void loadPersistent();
    void savePersistent();

    void setSeed(const QString& v){
        _seed = v;
        loadPersistent();
    }

private:
    QMenu* m_menu;

    QString _seed;
    QList<QString> s_recentBarcodes;
};

