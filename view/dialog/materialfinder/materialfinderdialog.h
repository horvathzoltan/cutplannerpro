#pragma once

#include "materials/model/material_master.h"
#include "materials/view/material_cell_generator.h"
#include <QDialog>
#include <QVector>
#include <QUuid>
#include <QStyledItemDelegate>
#include <QPainter>


namespace Ui {
class MaterialFinderDialog;
}

struct MaterialFinderInput {
    QUuid materialId;
    int minLen;
    int maxLen;
};

class MaterialFinderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MaterialFinderDialog(QWidget* parent = nullptr);
    ~MaterialFinderDialog();

    MaterialFinderInput getInput() const;

private:
    Ui::MaterialFinderDialog* ui;


};


