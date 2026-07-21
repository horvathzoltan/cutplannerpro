#pragma once

#include "../../../model/cutting/plan/request.h"
#include "view/dialog/dialogmode.h"
#include <QDialog>
#include <QLabel>
#include <QToolButton>
#include <QUuid>
#include "series_matrix_view.h"


namespace Ui {
class AddInputDialog;
}

enum class ReferenceState {
    NewReference,   // nincs HEAD, nincs ITEM
    HeadOnly,       // van HEAD, nincs ITEM
    FullRequest     // van HEAD, van ITEM
};

enum class EditMode {
    None,       // nézelődés
    HeadEdit,   // HEAD szerkesztés
    ItemEdit    // ITEM szerkesztés
};


class AddInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddInputDialog(QWidget *parent = nullptr,
                            DialogMode mode = DialogMode::Create,
                            const Cutting::Plan::Request* initial = nullptr,
                            SeriesMatrixView* matrix = nullptr);
    ~AddInputDialog();

    QUuid selectedMaterialId() const;
    int length() const;
    int quantity() const;

    QString ownerName() const;
    QString externalReference() const;

    void accept() override;
    void reject() override;
    Cutting::Plan::Request getModel() const;
    bool wasShiftEnter() const { return _shiftEnterAccepted; }
    bool shouldRepeat();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::AddInputDialog *ui;
    void populateMaterialCombo();
    bool validateInputs();
    SeriesMatrixView* _matrix = nullptr;
    bool _suppressPreview = false;

    // ⭐ Tételszám UI elemek (egy layouton)
    QLabel* lblReferenceBig = nullptr;
    QToolButton* btnEditReference = nullptr;
    QPushButton* btnNextRef = nullptr;
    QPushButton* btnNextMaterial = nullptr;
    QPushButton* btnFirstRef = nullptr;

    // ⭐ Kétmódú workflow metódusok
    void enterReferenceEditMode();              // nincs tételszám → beírás
    void loadReference(const QString& ref); // van tételszám → mutatás + nav

    void lockAllFieldsUntilReference();

    void onQuantityChanged(int totalPieces);
    void updateSliderLabels();

    QUuid current_requestId;

    //int _lengthHint = -1;
    QTimer* _lengthDebounceTimer = nullptr;
    QTimer* _colorDebounceTimer = nullptr;

    struct HeadFields {
        QString owner;
        QDate   due;
        QString color;
        QString surfaceCode;
        QUuid   typeId;
        QUuid   subtypeId;

        int quantity = 1;
        int leftCount = 0;
        int rightCount = 0;
    };


    static QString s_lastExternalRef;
    static bool s_lastRepeat;
    static QSet<QString> s_ownerCache;
    inline static const QString OWNER_CACHE_FN = "owner_cache.csv";

    bool _shiftEnterAccepted = false;

    DialogMode _mode = DialogMode::Create;

    struct BOM_Model {
        QVector<QUuid> bomList;        // aktuális ajánlott BOM (családonként 1)
        QSet<QUuid> addedMaterials;    // az adott externalReference-hez már rögzített anyagok
        QUuid lastSuggestedMaterial;   // utoljára ajánlott anyag ID
    };

    BOM_Model _bomModel;
    EditMode _editMode;

    void setHeadEditable(bool editable);
    void applyRequestToWidgets(const Cutting::Plan::Request& req);
    void applySide(HandlerSide side);
    void loadOwnerCache();
    void saveOwnerCache();

    void applyInitialFocus();
    QString computeNextReference();

    void initializeDialog();

    void setOwnerEditable(bool editable);
    void setDateEditable(bool editable);
    void setSideEditable(bool editable);
    void setColorEditable(bool editable);
    void setQuantityEditable(bool editable);

    void applyOwnerFromRequest(const Cutting::Plan::Request& req);
    void applyReferenceFromRequest(const Cutting::Plan::Request& req);
    void applyDateFromRequest(const Cutting::Plan::Request& req);
    void applyColorFromRequest(const Cutting::Plan::Request& req);
    void applyMaterialFromRequest(const Cutting::Plan::Request& req);

    void setReferenceEditable(bool editable);

    QString computeNextItemNumber();

    QUuid selectedProductTypeId() const;
    QUuid selectedProductSubtypeId() const;
    void setSelectedProductTypeId(const QUuid &typeId);
    void setSelectedProductSubtypeId(const QUuid &subtypeId);
    void applyProductTypeFromRequest(const Cutting::Plan::Request &req);
    void applyProductSubtypeFromRequest(const Cutting::Plan::Request &req);
    void setProductTypeEditable(bool editable);
    void setProductSubtypeEditable(bool editable);

    void populateSurfaceCombo();
    void setSurfaceEditable(bool editable);
    void applySurfaceFromRequest(const Cutting::Plan::Request &req);
    void updateColorPreview();

    void applySideFromRequest(const Cutting::Plan::Request &req);
    void resetUiForNextReference();
    void resetUiForExistingReference();
    void resetUiForNewReference();
    void loadHeadFields(const QString &ref);
    void updateHeadFieldsInRegistry(const QString &ref);
    AddInputDialog::HeadFields headFromRegistry(const QString& ref) const;
    AddInputDialog::HeadFields currentHeadFromDialog() const;
    bool headFieldsDiffer(const HeadFields &a, const HeadFields &b) const;

    QUuid computeNextMaterialForCurrentRef();
    void applyLengthFromRequest(const Cutting::Plan::Request &req);
    void applyQuantityFromRequest(const Cutting::Plan::Request &req);
    void applyWidth(const Cutting::Plan::Request& req);
    void applyHeight(const Cutting::Plan::Request& req);
    void applyFields_Head(const Cutting::Plan::Request &r);
    void applyFields_Item(const Cutting::Plan::Request &req);
    void initializeBomModel(const QString &ref);
    ReferenceState getReferenceState(const QString &ref);
    void applyReferenceState(ReferenceState state);
    void setLengthEditable(bool editable);
    void setMaterialEditable(bool editable);
    void setItemEditable(bool editable);
    void applySide_Slider(int l, int r);
    void refreshBom();
    void updateAttributePanel();
    void applyAttributes(const Cutting::Plan::Request &r);
    void groupboxAttributes_hide();
    void groupboxAttributes_show();
    QString currentProductTypeCode() const;
    QString currentProductSubtypeCode() const;
private slots:
    void on_btn_MaterialSearch_clicked();
    void on_btn_Reset_clicked();
    void onProductTypeChanged(bool checked);
    void on_btnEditReference_clicked(bool checked);

signals:
    void seriesContextChanged(const QString& owner,
                              const QString& externalRefPrefix);

};
