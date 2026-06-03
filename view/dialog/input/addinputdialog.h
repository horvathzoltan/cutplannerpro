#pragma once

#include "../../../model/cutting/plan/request.h"
#include <QDialog>
#include <QUuid>

namespace Ui {
class AddInputDialog;
}

class AddInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddInputDialog(QWidget *parent = nullptr);
    ~AddInputDialog();

    QUuid selectedMaterialId() const;
    int length() const;
    int quantity() const;

    QString ownerName() const;
    QString externalReference() const;

    void accept() override;
    Cutting::Plan::Request getModel() const;
    void setModel(const Cutting::Plan::Request& request);
    bool wasShiftEnter() const { return _shiftEnterAccepted; }

protected:
    void keyPressEvent(QKeyEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::AddInputDialog *ui;
    void populateMaterialCombo();
    bool validateInputs();
    Subtype parseSubtypeFromRadioButtons() const;

    void onQuantityChanged(int totalPieces);
    void updateHandlerSideControls();
    void updateSliderLabels();

    QUuid current_requestId;

    static QString s_lastOwnerName;
    static QString s_lastExternalRef;
    static QUuid   s_lastMaterialId;
    static Subtype s_lastSubtype;

    bool _shiftEnterAccepted = false;
    QString _sliderInputBuffer;
};
