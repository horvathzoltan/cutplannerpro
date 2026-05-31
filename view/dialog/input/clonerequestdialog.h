#pragma once
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QUuid>

struct CloneParams {
    QUuid newMaterialId;
    int delta;
    QString tag;
};

struct MaterialRuleWidgets {
    QUuid originalMaterialId;
    QLabel* label;
    QComboBox* combo;
    QSpinBox* spin;
};

struct CloneMaterialRule {
    QUuid originalMaterialId;
    QUuid newMaterialId;   // null = no change
    int delta;             // 0 = no change
};

namespace Ui { class CloneRequestDialog; }

class CloneRequestDialog : public QDialog {
    Q_OBJECT
public:
    explicit CloneRequestDialog(QWidget* parent = nullptr);
    ~CloneRequestDialog();
    QVector<CloneMaterialRule> result() const;
    QString tag() const;

    bool validateInputs();
    void accept() override;
private:
    Ui::CloneRequestDialog* ui;

    QVector<MaterialRuleWidgets> _materialRules;
};
