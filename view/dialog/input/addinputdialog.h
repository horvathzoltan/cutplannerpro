#pragma once

#include "../../../model/cutting/plan/request.h"
#include "view/dialog/dialogmode.h"
#include <QDialog>
#include <QUuid>


namespace Ui {
class AddInputDialog;
}

class AddInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddInputDialog(QWidget *parent = nullptr, DialogMode mode = DialogMode::Create);
    ~AddInputDialog();

    QUuid selectedMaterialId() const;
    int length() const;
    int quantity() const;

    QString ownerName() const;
    QString externalReference() const;

    void accept() override;
    void reject() override;
    Cutting::Plan::Request getModel() const;
    void setModel(const Cutting::Plan::Request& request);
    bool wasShiftEnter() const { return _shiftEnterAccepted; }

    bool shouldRepeat();

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


    static QString s_lastExternalRef;
    static bool s_lastRepeat;
    static QSet<QString> s_ownerCache;
    inline static const QString OWNER_CACHE_FN = "owner_cache.csv";
    inline static const QString CONTEXT_CACHE_FN = "context_cache.csv";

    bool _shiftEnterAccepted = false;
    QString _sliderInputBuffer;

    DialogMode _mode = DialogMode::Create;

    struct RequestContext {
        QString ownerName;     // Megrendelő neve
        QDate   dueDate;       // Határidő
        Subtype subtype;       // Alap / Rugós / Tetőtéri
        HandlerSide side;      // Bal / Jobb (egy napháló motor oldala nem változik)
        QUuid   defaultMaterialId; // Opcionális: legutóbb használt anyag ehhez a tételszámhoz
    };

    static QMap<QString, RequestContext> _contexts;

    void applyContextToWidgets(const RequestContext& ctx);
    void setContextEditable(bool editable);
    void applyRequestToWidgets(const Cutting::Plan::Request& req);
    void applySubtype(Subtype t);
    void applySide(HandlerSide side);
    void loadOwnerCache();
    void saveOwnerCache();
    void loadContextMap();
    void saveContextMap();
private slots:
    void on_btn_MaterialSearch_clicked();
    void on_btn_Reset_clicked();

};
