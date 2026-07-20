#include "material_selector.h"
#include "materials/registry/material_registry.h"
#include "materials/model/material_family_utils.h"

#include <model/registries/stockregistry.h>

#include <product/registry/product_type_registry.h>

struct Candidate {
    QUuid id;
    MaterialSelector::ScoreBreakdown breakdown;

    int totalScore() const {
        return breakdown.total();
    }
};


QVector<QUuid> MaterialSelector::rankMaterials(
    const QVector<QUuid>& bomList,
    const Cutting::Plan::Request& req)
{
    qDebug() << "=== MaterialSelector INPUT ===";
    qDebug() << "req.requiredColor.code =" << req.requiredColor.code();
    qDebug() << "req.requiredColor.lightness =" << req.requiredColor.lightness();
    qDebug() << "req.requiredColor.isValid =" << req.requiredColor.isValid();
    qDebug() << "req.requiredLength =" << req.requiredLength;
    qDebug() << "req.quantity =" << req.quantity;
    qDebug() << "req.productTypeId =" << req.productTypeId;
    qDebug() << "req.productSubtypeId =" << req.productSubtypeId;

    // qDebug() << "BOM list (input):";
    // for (auto id : bomList) {
    //     const MaterialMaster* m = MaterialRegistry::instance().findById(id);
    //     if (m)
    //         qDebug().nospace()
    //             << "  " << m->barcode
    //             << "  name=" << m->name
    //             << "  color=" << m->color.code()
    //             << "  family=" << MaterialFamilyUtils::toString(m->family)
    //             << "  diameter=" << m->diameter_mm
    //             << "  stockLength=" << m->stockLength_mm;
    // }
    qDebug() << "=== END INPUT ===";

    QVector<Candidate> ranked;
    // 1) Családonkénti csoportosítás
    QMap<MaterialFamily, QVector<Candidate>> groups;

    for (auto id : bomList) {
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(id);
        if (!mat)
            continue;

        ScoreBreakdown bd;

        // ------------------------------------------------------------
        // ⭐ 1) SZÍN PREFERENCIA — gyártási valóság szerint
        // ------------------------------------------------------------
        bool reqHasColor = req.requiredColor.isValid() &&
                           !req.requiredColor.code().trimmed().isEmpty();

        bool matHasColor = mat->color.isValid() &&
                           !mat->color.code().trimmed().isEmpty();

        if (reqHasColor)
        {
            double reqLight = req.requiredColor.lightness();
            double matLight = mat->color.lightness();
            double diff = std::abs(reqLight - matLight);

            // ⭐ Festett termék - 1. helyezett: színegyezéssel
            if(req.requiredColor.code().compare(mat->color.code(), Qt::CaseInsensitive) == 0){
                bd.colorExact += 200; // brutál előny a pontos egyezésre
            }
            // ⭐ Festett termék → 2. holtversenyben: natúr anyag a legjobb
            else if (!matHasColor) {
                bd.colorExact += 150;
            }
            else {

                // ⭐ Festett termék → világosság alapján rangsorolunk
                if (diff < 0.10) {
                    bd.colorExact += 120;   // nagyon hasonló → kiváló alap
                }
                else if (diff < 0.25) {
                    bd.colorExact += 40;    // közepesen hasonló → jó
                }
                else {
                    bd.colorExact -= 80;    // nagyon eltérő → kerülendő
                }
            }
        }
        else
        {
            // ⭐ Natúr termék → natúr anyag enyhén preferált
            if (!matHasColor)
                bd.colorExact += 50;
        }


        // ------------------------------------------------------------
        // ⭐ 2) TENGELY PREFERENCIA (hosszfüggő)
        // ------------------------------------------------------------
        if (mat->family == MaterialFamily::Tengely &&
            ProductTypeRegistry::instance().findById(req.productTypeId)->code == "NP")
        {
            int L = req.fullWidth_mm;

            if (L >= 3500) {
                if (mat->diameter_mm >= 78)
                    bd.axisPref += 300;
                else
                    bd.axisPref -= 100;
            } else {
                if (mat->diameter_mm == 70)
                    bd.axisPref += 150;
            }
        }

        // ------------------------------------------------------------
        // ⭐ 2) KÉSZLET PREFERENCIA — minden anyagra
        // ------------------------------------------------------------
        auto stockEntries = StockRegistry::instance().findByMaterialId(mat->id);

        bool inStock = false;
        for (const auto& se : stockEntries) {
            if (se.quantity > 0) {
                inStock = true;
                break;
            }
        }

        if (inStock)
            bd.stockPref += 200;
        else
            bd.stockPref -= 200;


        Candidate c;
        c.id = id;
        c.breakdown = bd;

        ranked << c;
        groups[mat->family].append(c);
    }

    // ------------------------------------------------------------
    // ⭐ Rangsorolás
    // ------------------------------------------------------------
    // 2) Családon belüli sort
    for (auto& vec : groups) {
        std::sort(vec.begin(), vec.end(),
                  [](const Candidate& a, const Candidate& b){
                        return a.totalScore() > b.totalScore();
                  });
    }

    // 3) BOM sorrend visszaállítása
    QVector<QUuid> result;
    for (auto id : bomList) {
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(id);
        if (!mat)
            continue;

        auto& vec = groups[mat->family];
        if (!vec.isEmpty()) {
            result << vec.first().id;
            vec.removeFirst();
        }
    }


    // ------------------------------------------------------------
    // ⭐ DEBUG: listázzuk ki a rangsorolt anyagokat
    // ------------------------------------------------------------

    qDebug() << "MaterialSelector ranked result:";
    for (const auto& c : ranked) {
        const MaterialMaster* m = MaterialRegistry::instance().findById(c.id);
        if (!m) continue;

        qDebug().nospace()
            << "  total=" << c.totalScore()
            << "  [exact=" << c.breakdown.colorExact
            << ", nat=" << c.breakdown.colorNat
            << ", light=" << c.breakdown.colorLightness
            << ", penalty=" << c.breakdown.colorPenalty
            << ", axis=" << c.breakdown.axisPref
            << ", stock=" << c.breakdown.stockPref
            << "]"
            << "  barcode=" << m->barcode
            << "  name=" << m->name
            << "  color=" << m->color.code()
            << "  diameter=" << m->diameter_mm
            << "  stockLength=" << m->stockLength_mm;

    }


    return result;
}

QUuid MaterialSelector::selectPreferred(
    const QVector<QUuid>& bomList,
    const Cutting::Plan::Request& req)
{
    auto ranked = rankMaterials(bomList, req);
    return ranked.isEmpty() ? QUuid() : ranked.first();
}
