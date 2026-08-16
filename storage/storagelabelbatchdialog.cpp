#include "storagelabelbatchdialog.h"
#include "storage/utils/storageutils.h"
#include "ui_storagelabelbatchdialog.h"

#include "presenter/storagepresenter.h"

StorageLabelBatchDialog::StorageLabelBatchDialog(QWidget* parent)
    : QDialog(parent),
    ui(new Ui::StorageLabelBatchDialog)
{
    ui->setupUi(this);

    ui->treeWidget->setHeaderHidden(true);

    populateTree();

    connect(ui->treeWidget, &QTreeWidget::itemChanged,
            this, &StorageLabelBatchDialog::onItemChanged);
}

StorageLabelBatchDialog::~StorageLabelBatchDialog()
{
    delete ui;
}

void StorageLabelBatchDialog::populateTree()
{
    ui->treeWidget->clear();

    const auto& storages = StorageRegistry::instance().readAll();
    QHash<QUuid, QTreeWidgetItem*> nodes;

    // --- Node-ok létrehozása ---
    for (const auto& s : storages) {

        QString logistic = StorageRegistry::instance().logisticBarcode(s.id);
        bool isLeaf = StorageUtils::isLeaf(s.id);

        QString icon = s.type.icon();   // 🏬 / 🗃️ / 🗄️ / 📦
        QString text = QString("%1 %2 (%3)")
                           .arg(icon)
                           .arg(s.name)
                           .arg(logistic);

        if (isLeaf) {
            text = QString("🌿 %1").arg(text);
        }

        auto* item = new QTreeWidgetItem();
        item->setText(0, text);

        // checkbox
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Unchecked);

        // FONT: csomópont félkövér, levél vékony
        QFont f = item->font(0);
        f.setBold(!isLeaf);
        item->setFont(0, f);

        // eltároljuk a StorageEntry pointert
        item->setData(0, Qt::UserRole, QVariant::fromValue(const_cast<StorageEntry*>(&s)));

        nodes[s.id] = item;
    }

    // --- Hierarchia felépítése ---
    for (const auto& s : storages) {
        if (s.parentId.isNull()) {
            ui->treeWidget->addTopLevelItem(nodes[s.id]);
        } else {
            nodes[s.parentId]->addChild(nodes[s.id]);
        }
    }

    ui->treeWidget->expandAll();
}

void StorageLabelBatchDialog::checkChildrenRecursive(QTreeWidgetItem* item, Qt::CheckState st)
{
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem* child = item->child(i);
        child->setCheckState(0, st);
        checkChildrenRecursive(child, st);   // ⭐ mélyebb szintek is pipálódnak
    }
}


void StorageLabelBatchDialog::onItemChanged(QTreeWidgetItem* item, int)
{
    ui->treeWidget->blockSignals(true);

    Qt::CheckState st = item->checkState(0);

    // ⭐ Szülő → gyerek (rekurzív)
    checkChildrenRecursive(item, st);

    ui->treeWidget->blockSignals(false);

    // ⭐ Gyerek → szülő (maradhat a régi)
    QTreeWidgetItem* parent = item->parent();
    if (parent) {

        ui->treeWidget->blockSignals(true);

        int checked = 0;
        int unchecked = 0;

        for (int i = 0; i < parent->childCount(); ++i) {
            Qt::CheckState cs = parent->child(i)->checkState(0);
            if (cs == Qt::Checked) checked++;
            else if (cs == Qt::Unchecked) unchecked++;
        }

        if (checked == parent->childCount())
            parent->setCheckState(0, Qt::Checked);
        else if (unchecked == parent->childCount())
            parent->setCheckState(0, Qt::Unchecked);
        else
            parent->setCheckState(0, Qt::PartiallyChecked);

        ui->treeWidget->blockSignals(false);
    }
}


void StorageLabelBatchDialog::collectChecked(QTreeWidgetItem* item,
                                             QList<StorageEntry*>& out)
{
    if (item->checkState(0) == Qt::Checked) {
        QVariant v = item->data(0, Qt::UserRole);
        if (v.isValid())
            out.append(v.value<StorageEntry*>());
    }

    for (int i = 0; i < item->childCount(); ++i)
        collectChecked(item->child(i), out);
}

void StorageLabelBatchDialog::accept()
{
    m_selected.clear();
    collectChecked(ui->treeWidget->invisibleRootItem(), m_selected);
    QDialog::accept();
}

