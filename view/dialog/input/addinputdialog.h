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

    enum class ContextMode {
        Editing,
        Existing,    // létező tételszám
        Sequential,  // sorozatbevitel (haladunk előre)
        NewOrder     // új megrendelő
    };

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
    QString _nextSuggestedRef;
    QString _originalReference;

    DialogMode _mode = DialogMode::Create;
    ContextMode _contextMode = ContextMode::NewOrder;

    struct RequestContext {
        QString ownerName;     // Megrendelő neve
        QDate   dueDate;       // Határidő
        Subtype subtype;       // Alap / Rugós / Tetőtéri
        HandlerSide side;      // Bal / Jobb (egy napháló motor oldala nem változik)
        QUuid   defaultMaterialId; // Opcionális: legutóbb használt anyag ehhez a tételszámhoz
        QString color;
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

    ContextMode detectContextMode(const QString& ref);
    void applyContextMode(ContextMode mode, const QString& ref);

    void applyExistingContext(const QString& ref);
    void applySequentialContext(const QString& ref);
    void applyNewOrderContext();

    void resetForNewOrder();
    void resetForSequential();

    void applyInitialFocus();
    QString computeNextReference();

    void updateContextModeLabel();
    void initializeDialog();

    void applyOwnerAndDate(const RequestContext& ctx);
    void applySubtypeFromContext(const RequestContext& ctx);
    void applySideFromContext(const RequestContext& ctx);
    void applyMaterialFromContext(const RequestContext& ctx);
    void applyColorFromContext(const RequestContext& ctx);

    void setOwnerEditable(bool editable);
    void setDateEditable(bool editable);
    void setSubtypeEditable(bool editable);
    void setSideEditable(bool editable);
    void setColorEditable(bool editable);
    void setQuantityEditable(bool editable);

    void applyOwnerFromRequest(const Cutting::Plan::Request& req);
    void applyReferenceFromRequest(const Cutting::Plan::Request& req);
    void applyDateFromRequest(const Cutting::Plan::Request& req);
    void applyColorFromRequest(const Cutting::Plan::Request& req);
    void applySubtypeFromRequest(const Cutting::Plan::Request& req);
    void applySideFromRequest(const Cutting::Plan::Request& req);
    void applyMaterialFromRequest(const Cutting::Plan::Request& req);
    void applyLengthAndQuantityFromRequest(const Cutting::Plan::Request& req);

    void setReferenceEditable(bool editable);

private slots:
    void on_btn_MaterialSearch_clicked();
    void on_btn_Reset_clicked();

};
