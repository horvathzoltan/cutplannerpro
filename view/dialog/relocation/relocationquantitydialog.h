#pragma once

#include <QDialog>
#include "model/relocation/relocationquantityrow.h"
#include "model/relocation/relocationquantitymodel.h"

namespace Ui {
class RelocationQuantityDialog;
}

class RelocationQuantityDialog : public QDialog {
    Q_OBJECT

public:
    explicit RelocationQuantityDialog(QWidget* parent = nullptr);
    ~RelocationQuantityDialog();

    void setRows(const QVector<RelocationQuantityRow>& rows);
    QVector<RelocationQuantityRow> getRows() const;

private slots:
    void updateSummary();
    //void distributeEvenly();
    void validate();

private:
    Ui::RelocationQuantityDialog* ui;
    RelocationQuantityModel model;
};
