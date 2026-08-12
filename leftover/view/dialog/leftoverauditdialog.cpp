#include "leftoverauditdialog.h"

#include "storage/registry/storageregistry.h"
#include "materials/registry/material_registry.h"
#include "leftover/registry/leftoverstockregistry.h"

#include <QSet>

LeftoverAuditDialog::LeftoverAuditDialog(QWidget* parent)
    : QDialog(parent),
    ui(new Ui::LeftoverAuditDialog)
{
    ui->setupUi(this);

    loadStorages();
    loadMaterials();
}

LeftoverAuditDialog::~LeftoverAuditDialog()
{
    delete ui;
}

void LeftoverAuditDialog::loadStorages()
{
    // 1) leftoverek lekérése
    auto leftovers = LeftoverStockRegistry::instance().readAll();

    // 2) storageId-k kigyűjtése
    QSet<QUuid> storageIds;
    for (const auto& e : leftovers) {
        storageIds.insert(e.storageId);
    }

    // 3) storage registry lekérése
    auto storages = StorageRegistry::instance().readAll();

    // 4) csak azok a storage-ek, ahol leftover van
    for (const auto& s : storages) {
        if (storageIds.contains(s.id)) {
            ui->comboBox_Storages->addItem(s.name, s.id);
        }
    }
}

void LeftoverAuditDialog::loadMaterials()
{
    // 1) leftoverek lekérése
    auto leftovers = LeftoverStockRegistry::instance().readAll();

    // 2) materialId-k kigyűjtése
    QSet<QUuid> materialIds;
    for (const auto& e : leftovers) {
        materialIds.insert(e.materialId);
    }

    // 3) material registry lekérése
    auto materials = MaterialRegistry::instance().readAll();

    // 4) csak azok az anyagok, amelyekből leftover van
    for (const auto& m : materials) {
        if (materialIds.contains(m.id)) {   // MaterialMaster.id !!!
            QListWidgetItem* item =
                new QListWidgetItem(m.toDisplay(), ui->listWidget_Materials);

            item->setData(Qt::UserRole, m.id);
            item->setCheckState(Qt::Unchecked);
        }
    }
}

QUuid LeftoverAuditDialog::selectedStorage() const
{
    return ui->comboBox_Storages->currentData().toUuid();
}

QVector<QUuid> LeftoverAuditDialog::selectedMaterials() const
{
    QVector<QUuid> ids;

    for (int i = 0; i < ui->listWidget_Materials->count(); ++i) {
        QListWidgetItem* item = ui->listWidget_Materials->item(i);
        if (item->checkState() == Qt::Checked) {
            ids.append(item->data(Qt::UserRole).toUuid());
        }
    }

    return ids;
}
