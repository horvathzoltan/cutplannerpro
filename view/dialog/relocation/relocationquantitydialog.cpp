#include "relocationquantitydialog.h"
#include "ui_relocationquantitydialog.h"

#include <QPushButton>
#include <QSpinBox>

RelocationQuantityDialog::RelocationQuantityDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::RelocationQuantityDialog) {
    ui->setupUi(this);

    //connect(ui->btnDistributeEvenly, &QPushButton::clicked, this, &RelocationQuantityDialog::distributeEvenly);
    //connect(ui->btnOk, &QPushButton::clicked, this, &RelocationQuantityDialog::accept);
    //connect(ui->btnCancel, &QPushButton::clicked, this, &RelocationQuantityDialog::reject);
}

RelocationQuantityDialog::~RelocationQuantityDialog() {
    delete ui;
}

void RelocationQuantityDialog::setRows(const QVector<RelocationQuantityRow>& rows) {
    model.rows = rows;

    auto* table = ui->tableQuantities;
    table->clearContents();   // csak a cellák ürülnek, az oszlopok és a fejléc marad    //table->setColumnCount(3);
    //table->setHorizontalHeaderLabels({tr("Tárhely"), tr("Elérhető"), tr("Mozgatott")});
    table->setRowCount(rows.size());
    table->horizontalHeader()->setStretchLastSection(true);

    for (int i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];

        // 1. oszlop: tárhely neve
        table->setItem(i, 0, new QTableWidgetItem(r.storageName));

        // 2. oszlop: elérhető mennyiség (forrásnál értelmes, célnál lehet 0 vagy üres)
        auto* availItem = new QTableWidgetItem(QString::number(r.isTarget ? 0 : r.available));
        availItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        table->setItem(i, 1, availItem);

        // 3. oszlop: mozgatott mennyiség (szerkeszthető spinbox)
        auto* spin = new QSpinBox(table);
        spin->setMinimum(0);
        spin->setMaximum(r.isTarget ? 999999 : r.available); // forrásnál ne lehessen többet vinni, mint ami van
        spin->setValue(r.selected);

        // meta adatok
        spin->setProperty("rowIndex", i);
        spin->setProperty("isTarget", r.isTarget);

        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int){
            updateSummary();
        });

        table->setCellWidget(i, 2, spin);
    }

    updateSummary();
}

QVector<RelocationQuantityRow> RelocationQuantityDialog::getRows() const {
    QVector<RelocationQuantityRow> out;
    auto* table = ui->tableQuantities;
    out.reserve(table->rowCount());

    for (int i = 0; i < table->rowCount(); ++i) {
        RelocationQuantityRow r;
        r.storageName = table->item(i, 0)->text();
        r.available   = table->item(i, 1)->text().toInt();

        if (auto* w = table->cellWidget(i, 2)) {
            if (auto* spin = qobject_cast<QSpinBox*>(w)) {
                r.selected = spin->value();
                r.isTarget = spin->property("isTarget").toBool();
            }
        }
        out.append(r);
    }
    return out;
}


void RelocationQuantityDialog::updateSummary() {
    auto* table = ui->tableQuantities;
    int total = 0;

    for (int i = 0; i < table->rowCount(); ++i) {
        if (auto* w = table->cellWidget(i, 2)) {
            if (auto* spin = qobject_cast<QSpinBox*>(w)) {
                total += spin->value();
            }
        }
    }

    ui->labelSummaryRight->setText(QString("Összesen: %1").arg(total));
    validate();
}


// void RelocationQuantityDialog::distributeEvenly() {
//     model.distributeEvenly(model.totalSelected());
//     // TODO: update tableQuantities spinboxes
//     updateSummary();
// }

void RelocationQuantityDialog::validate() {
    //ui->btnOk->setEnabled(model.isValid());
}
