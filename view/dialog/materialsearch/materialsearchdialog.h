#pragma once

#include <QDialog>
#include <QVector>
#include <QTimer>
#include <QListView>
#include <QButtonGroup>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>

#include "materials/model/material_master.h"

struct MaterialSelection {
    QUuid id;
    MaterialMaster master;
};

class MaterialSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialSearchDialog(QWidget* parent = nullptr);
    ~MaterialSearchDialog();

    MaterialSelection selection() const;

private:
    // UI elemek
    QWidget* colorFilterPanel;
    QButtonGroup* colorButtons;
    QLineEdit* searchEdit;
    QListView* resultList;

    // Model
    QStandardItemModel* model;
    QVector<MaterialMaster> allMaterials;

    // Debounce timer
    QTimer debounce;


    QWidget* categoryFilterPanel;
    QButtonGroup* categoryButtons;

    QString selectedCategory() const;
    QString prefixOf(const QString& name) const;
    void buildCategoryButtons();


    // Belső logika
    void buildColorButtons();
    void applyFilter(const QString& text);
    QString selectedColor() const;
    void addSeparator(const QString &title);
    QStringList extractTokens(const QString &name) const;
};
