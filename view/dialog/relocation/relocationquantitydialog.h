#pragma once

#include <QDialog>
#include "model/relocation/relocationquantityrow.h"
#include "model/relocation/relocationquantitymodel.h"

namespace Ui {
class RelocationQuantityDialog;
}

enum class QuantityDialogMode { Source, Target };

class RelocationQuantityDialog : public QDialog {
    Q_OBJECT

public:
    explicit RelocationQuantityDialog(QWidget* parent = nullptr);
    ~RelocationQuantityDialog();

    void setMode(QuantityDialogMode mode) { _mode = mode; };
    void setRows(const QVector<RelocationQuantityRow>& rows, int planned, int selectedFromSources);
    QVector<RelocationQuantityRow> getRows() const;

    bool isValid() const;
private slots:
    void updateSummary();
    //void distributeEvenly();
    void validate();

private:
    Ui::RelocationQuantityDialog* _ui;
    RelocationQuantityModel _model;

    int _plannedQuantity = 0;
    int _totalSelectedFromSources = 0;
    int _totalDistributedToTargets = 0;
    int _selectedFromSources = -1;
    QuantityDialogMode _mode = QuantityDialogMode::Source; // default
};
