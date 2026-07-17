#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>

#include <materials/registry/material_registry.h>

#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>
#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <materials/model/material_family_utils.h>

#include "series_state.h"
#include "../../../presenter/CuttingPresenter.h"
#include "../../../model/cutting/plan/request.h"

// class ColorlessFamilyDetector {
// public:
//     QSet<MaterialFamily> colorlessFamilies;
//     QHash<MaterialFamily, QUuid> representative;
//     QHash<QUuid, QHash<QUuid, QHash<MaterialFamily, QUuid>>> repByTypeSubtype;

//     void build() {
//         colorlessFamilies.clear();

//         // összes anyag bejárása
//         auto mats = MaterialRegistry::instance().readAll();

//         // először feltételezzük, hogy minden család colorless
//         QSet<MaterialFamily> allFamilies;
//         for (const auto& m : mats)
//             allFamilies.insert(m.family);

//         colorlessFamilies = allFamilies;

//         // ha bármely családnak van színes tagja → nem colorless
//         for (const auto& m : mats) {
//             bool hasColor = m.color.isValid() &&
//                             !m.color.code().trimmed().isEmpty();
//             if (hasColor)
//                 colorlessFamilies.remove(m.family);
//         }


//         // reprezentáns kiválasztása minden colorless családhoz
//         for (MaterialFamily fam : colorlessFamilies) {
//             QUuid bestId;
//             int bestLength = -1;

//             for (const auto& m : mats) {
//                 if (m.family != fam)
//                     continue;

//                 int length = m.stockLength_mm; // vagy m.size, m.width, m.height – ami releváns
//                 if (length > bestLength) {
//                     bestLength = length;
//                     bestId = m.id;
//                 }
//             }

//             representative[fam] = bestId;
//         }

//         // ⭐ Terméktípus-altípus specifikus reprezentánsok felépítése
//         repByTypeSubtype.clear();

//         auto roles = MaterialRoleRegistry::instance().readAll();
//         // NOTE: ha van külön getter az összes role-ra, azt használd

//         for (const auto& r : roles) {

//             MaterialFamily fam = r.family;

//             // csak colorless családokra építünk lookupot
//             if (!colorlessFamilies.contains(fam))
//                 continue;

//             QString prefix = r.barcodePrefix.trimmed();
//             if (prefix.endsWith("*"))
//                 prefix.chop(1);

//             QUuid bestMatch;

//             for (const auto& m : mats) {
//                 if (m.family != fam)
//                     continue;

//                 if (m.barcode.startsWith(prefix)) {
//                     bestMatch = m.id;
//                     break;
//                 }
//             }

//             if (!bestMatch.isNull()) {
//                 auto* pp = ProductTypeRegistry::instance().findById(r.productTypeId);
//                 auto* ps = ProductSubtypeRegistry::instance().findById(r.productSubtypeId);

//                 QString type = pp?pp->name:"UNKNOWN";
//                 QString subtype = ps?ps->name:"UNKNOWN";

//                 repByTypeSubtype[r.productTypeId][r.productSubtypeId][fam] = bestMatch;

//                 auto* mm = MaterialRegistry::instance().findById(bestMatch);

//                 QString matLabel = mm ? mm->toDisplay() : "UNKNOWN";

//                 QString ff = MaterialFamilyUtils::toString(fam);

//                 zInfo(QString("REP LOOKUP: %1/%2 family=%3 → %4")
//                           .arg(type)
//                           .arg(subtype)
//                           .arg(ff)
//                           .arg(matLabel));

//             }
//         }



//         // zInfo("---- COLORLESS FAMILY DETECTOR ----");

//         // for (MaterialFamily fam : colorlessFamilies) {

//         //     QUuid rep = representative.value(fam);

//         //     // család + reprezentáns ID
//         //     zInfo(QString("FAMILY: %1  REPRESENTATIVE: %2")
//         //               .arg(static_cast<int>(fam))
//         //               .arg(rep.toString()));

//         //     // család tagjai részletesen
//         //     for (const auto& m : mats) {
//         //         if (m.family != fam)
//         //             continue;

//         //         QString line = QString("   MEMBER: %1  ID:%2  LEN:%3  DIAM:%4  COLOR:%5")
//         //                            .arg(m.name)
//         //                            .arg(m.id.toString())
//         //                            .arg(m.stockLength_mm)
//         //                            .arg(m.diameter_mm)
//         //                            .arg(m.color.code());

//         //         zInfo(line);
//         //     }
//         // }

//         // zInfo("----------------------------------");
//     }

//     bool isColorless(MaterialFamily fam) const {
//         return colorlessFamilies.contains(fam);
//     }

//     QUuid representativeOf(MaterialFamily fam) const {
//         return representative.value(fam);
//     }

// };

class SeriesMatrixView : public QWidget
{
    Q_OBJECT

public:
    explicit SeriesMatrixView(QWidget* parent,
                              CuttingPresenter* presenter);

    void updateMatrix(const ActiveSeries& active,
                      const QVector<Cutting::Plan::Request>& fullSeries);

    void setActiveReference(const QString &ref);
    void refreshAfterAdd(const QString &ref);

    void clearBomCache();

    void addFilledCell(const QString& ref, const QUuid& matId);
    void removeFilledCell(const QString& ref, const QUuid& matId);
    void updateFilledCell(const QString& oldRef, const QUuid& oldMat,
                          const QString& newRef, const QUuid& newMat);

    void addColumn(const QString& ref);
    void updateCell(int row, int col);   // a következő lépésben implementáljuk

    void addRow(const QUuid& matId);

    void setActiveSeries(const ActiveSeries& s) {_active = s;}
// public slots:
//     void onSeriesContextChanged(const QString& owner,
//                                 const QString& externalRefPrefix);

    QVector<QUuid> buildSectionedBomList();

    //QUuid nextBomMaterial(const QString& ref);

    //QString nextBomReference_2(const QString &currentRef);
    //QString nextBomReference(const QString &currentRef);
    //QString firstBomReference() const;
    void jumpToFirstReference();
signals:
    void matrixClosed();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    CuttingPresenter* _presenter = nullptr;

    //ColorlessFamilyDetector _colorless;

    ActiveSeries _active;
    QVector<Cutting::Plan::Request> _full;

    QTableWidget* _table = nullptr;

    void rebuildMatrix();
    void buildHeaders();
    void buildRows();
    void fillCells();

    QSet<QUuid> _actualMaterials;     // sorozatban ténylegesen előforduló anyagok
    QSet<QUuid> _bomMaterialsSet;     // BOM anyagok halmaza

    void computeMaterialSets();
    //QVector<QUuid> generateBomMaterials(const Cutting::Plan::Request &req) const;
    const Cutting::Plan::Request *findRequestByExternalRef(const QString &ref) const;

    mutable QHash<QPair<QUuid, QUuid>, QVector<QUuid>> _bomCache;
    inline QPair<QUuid, QUuid> bomKey(const Cutting::Plan::Request& req) const {
        return qMakePair(req.productTypeId, req.productSubtypeId);
    }

    QSet<QPair<QString, QUuid>> _filledCache;

    int findColumnIndex(const QString& ref) const;

    NamedColor effectiveColorForReference(const QString &ref) const;
    QVector<QUuid> generateBomMaterials(const Cutting::Plan::Request &req, const NamedColor &effectiveColor) const;
};


