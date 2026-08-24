#include "bundlesplitdialog.h"
#include "ui_bundlesplitdialog.h"
#include <QSpinBox>
#include <QCheckBox>

BundleSplitDialog::BundleSplitDialog(QWidget* parent)
    : QDialog(parent),
    ui(new Ui::BundleSplitDialog)
{
    ui->setupUi(this);

    ui->tableComponents->setColumnCount(4);
    ui->tableComponents->setHorizontalHeaderLabels(
        {"Material", "Length", "New Length", "Remove"}
        );

    connect(ui->tableComponents, &QTableWidget::cellChanged,
            this, &BundleSplitDialog::updateSummary);
}

BundleSplitDialog::~BundleSplitDialog()
{
    delete ui;
}

void BundleSplitDialog::setModel(const LeftoverStockEntry& entry)
{
    _model = entry;
    populateTable();
    updateSummary();
}

void BundleSplitDialog::populateTable()
{
    auto& comps = _model.bundleComponentLengths;
    ui->tableComponents->setRowCount(comps.size());

    int row = 0;
    for (const auto& c : comps)
    {
        const MaterialMaster* mat = MaterialRegistry::instance().findById(c.materialId);
        QString matName = mat ? mat->barcode : "(?)";

        // Material
        auto* itemMat = new QTableWidgetItem(matName);
        itemMat->setFlags(Qt::ItemIsEnabled);
        ui->tableComponents->setItem(row, 0, itemMat);

        // Original length
        int origLen = (c.length_mm == -1 ? _model.availableLength_mm : c.length_mm);
        auto* itemOrig = new QTableWidgetItem(QString::number(origLen));
        itemOrig->setFlags(Qt::ItemIsEnabled);
        ui->tableComponents->setItem(row, 1, itemOrig);

        // New length (spinbox)
        auto* sb = new QSpinBox();
        sb->setMinimum(0);
        sb->setMaximum(_model.availableLength_mm);
        sb->setValue(origLen);
        ui->tableComponents->setCellWidget(row, 2, sb);

        // Remove checkbox
        auto* chk = new QCheckBox();
        chk->setChecked(false);
        ui->tableComponents->setCellWidget(row, 3, chk);

        row++;
    }
}

void BundleSplitDialog::updateSummary()
{
    int totalRemaining = 0;
    int totalRemoved = 0;

    int rows = ui->tableComponents->rowCount();
    for (int r = 0; r < rows; ++r)
    {
        auto* sb = qobject_cast<QSpinBox*>(ui->tableComponents->cellWidget(r, 2));
        auto* chk = qobject_cast<QCheckBox*>(ui->tableComponents->cellWidget(r, 3));

        if (!sb || !chk) continue;

        int len = sb->value();

        if (chk->isChecked())
            totalRemoved += len;
        else
            totalRemaining += len;
    }

    QString txt = QString(
                      "Eredeti leftover: %1 mm\n"
                      "Maradó komponensek összhossza: %2 mm\n"
                      "Kivett komponensek összhossza: %3 mm"
                      ).arg(_model.availableLength_mm)
                      .arg(totalRemaining)
                      .arg(totalRemoved);

    ui->lblSummary->setText(txt);
}

BundleSplitDialogResult BundleSplitDialog::getResult() const
{
    BundleSplitDialogResult out;
    out.leftoverId = _model.entryId;

    int rows = ui->tableComponents->rowCount();
    for (int r = 0; r < rows; ++r)
    {
        QUuid matId = _model.bundleComponentLengths[r].materialId;

        auto* sb = qobject_cast<QSpinBox*>(ui->tableComponents->cellWidget(r, 2));
        auto* chk = qobject_cast<QCheckBox*>(ui->tableComponents->cellWidget(r, 3));

        int newLen = sb ? sb->value() : 0;

        BundleComponentLength comp;
        comp.materialId = matId;
        comp.length_mm = newLen;

        if (chk && chk->isChecked())
            out.removedComponents.append(comp);
        else
            out.newComponents.append(comp);
    }

    return out;
}
