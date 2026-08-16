#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDialogButtonBox>

#include "storage/registry/storageregistry.h"

namespace Ui {
class StorageLabelBatchDialog;
}

class StorageLabelBatchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StorageLabelBatchDialog(QWidget* parent = nullptr);
    ~StorageLabelBatchDialog();
    QList<StorageEntry*> selectedStorages() const {return m_selected;};

private slots:
    void onItemChanged(QTreeWidgetItem* item, int column);
    void accept() override;

private:
    Ui::StorageLabelBatchDialog* ui;
    QList<StorageEntry*> m_selected;

    void populateTree();
    void collectChecked(QTreeWidgetItem* item, QList<StorageEntry*>& out);
    void checkChildrenRecursive(QTreeWidgetItem *item, Qt::CheckState st);
};
