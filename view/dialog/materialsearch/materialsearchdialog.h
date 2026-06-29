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
    explicit MaterialSearchDialog(
        QWidget* parent = nullptr,
        const QString& initialColor = "Nincs",
        const QString& initialType = "Mind",
        const QString& initialSubtype = "Mind",
        const QString& initialSearch = ""
        );

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

    // ⭐ ProductType kategória
    QButtonGroup* typeButtons;

    QString initColor;
    QString initType;
    QString initSubtype;
    QString initSearch;

    void buildTypeButtons();
    QString selectedType() const;

    // ⭐ ProductSubtype kategória
    QButtonGroup* subtypeButtons;

    QWidget* typePanel;        // Típusok panel
    QVBoxLayout* typeLayout;   // Típusok layout

    QWidget* subtypePanel;     // Altípusok panel
    QVBoxLayout* subtypeLayout;// Altípusok layout

    void buildSubtypeButtons();
    QString selectedSubtype() const;


    // Belső logika
    void buildColorButtons();
    void applyFilter(const QString& text);
    QString selectedColor() const;
    void addSeparator(const QString &title);
};
