#pragma once

#include <QDialog>
#include <QUuid>

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
    Ui::MaterialFinderDialog* _ui;

    int _lastAutoMin = -1;
    int _lastAutoMax = -1;
};
