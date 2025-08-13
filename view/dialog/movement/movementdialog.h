#pragma once

#include "model/movementdata.h"
#include <QDialog>
#include <QUuid>

QT_BEGIN_NAMESPACE
namespace Ui { class MovementDialog; }
QT_END_NAMESPACE

class MovementDialog : public QDialog {
    Q_OBJECT

public:
    explicit MovementDialog(QWidget* parent = nullptr);
    ~MovementDialog();

    void setSource(const QString& storageName, const QUuid& entryId, int availableQuantity);
    MovementData getMovementData() const;

private:
    Ui::MovementDialog* ui;
    QUuid sourceEntryId;
    int maxQuantity;
};
