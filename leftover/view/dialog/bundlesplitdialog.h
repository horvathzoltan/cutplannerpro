#pragma once

#include <QDialog>
#include <QVector>
#include <QUuid>
#include "leftover/model/leftoverstockentry.h"
#include "materialbundles/model/bundle_componentlength.h"

namespace Ui {
class BundleSplitDialog;
}

struct BundleSplitDialogResult {
    QUuid leftoverId;
    QVector<BundleComponentLength> newComponents;   // módosított komponensek
    QVector<BundleComponentLength> removedComponents; // kivett komponensek
};

class BundleSplitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BundleSplitDialog(QWidget* parent = nullptr);
    ~BundleSplitDialog();

    void setModel(const LeftoverStockEntry& entry);
    BundleSplitDialogResult getResult() const;

private:
    Ui::BundleSplitDialog* ui;
    LeftoverStockEntry _model;

    void populateTable();
    void updateSummary();
};
