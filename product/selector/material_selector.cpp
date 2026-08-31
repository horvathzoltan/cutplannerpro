#include "material_selector.h"
#include "materials/registry/material_registry.h"
#include "materials/model/material_family_utils.h"

#include "stock/registry/stockregistry.h"

#include <product/registry/product_type_registry.h>

#include <materialbundles/registry/bundle_registry.h>

struct Candidate {
    QUuid id;
    MaterialSelector::ScoreBreakdown breakdown;

    int totalScore() const {
        return breakdown.total();
    }
};


MaterialSelector::MaterialSelectionResult MaterialSelector::rankMaterials(
    const QVector<QUuid>& bomList,
    const Cutting::Plan::Request& req)
{
    MaterialSelectionResult r;

    // qDebug() << "=== MaterialSelector INPUT ===";
    // qDebug() << "req.requiredColor.code =" << req.requiredColor.code();
    // qDebug() << "req.requiredColor.lightness =" << req.requiredColor.lightness();
    // qDebug() << "req.requiredColor.isValid =" << req.requiredColor.isValid();
    // qDebug() << "req.requiredLength =" << req.requiredLength;
    // qDebug() << "req.quantity =" << req.quantity;
    // qDebug() << "req.productTypeId =" << req.productTypeId;
    // qDebug() << "req.productSubtypeId =" << req.productSubtypeId;

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
    // qDebug() << "=== END INPUT ===";

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
                    r.materialWarnings[id] << "Jelentős színeltérés";
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
            int H = req.fullHeight_mm;

            if (L >= 3500)
            {
                if (mat->diameter_mm >= 78)
                {
                    if (H <= 2500)
                        bd.axisPref += 300;   // preferált, ha befér
                    else
                        bd.axisPref -= 150;   // büntetés, ha nem fér be
                }
                else
                {
                    bd.axisPref -= 100;
                }
            }
            else
            {
                if (mat->diameter_mm == 70)
                    bd.axisPref += 150;
            }
        }

        // --------------------------------------------------------------
        // ⭐ 2) MOTOR PREFERENCIA (vászon terület - azaz tömeg függő)
        // --------------------------------------------------------------
        if (mat->family == MaterialFamily::Motor &&
            ProductTypeRegistry::instance().findById(req.productTypeId)->code == "NP")
        {
            int L = req.fullWidth_mm;
            int H = req.fullHeight_mm;

            double area_m2 = (L / 1000.0) * (H / 1000.0);

            // --- nyomaték kinyerése a barcode-ból ---
            // NP-MOT-10  → torque = 10
            // NP-MOT-20  → torque = 20
            QStringList parts = mat->barcode.split('-');
            int torque = 0;
            if (parts.size() >= 3 && parts[0] == "NP" && parts[1] == "MOT") {
                bool ok = false;
                torque = parts.last().toInt(&ok);
                if (!ok) torque = 0;
            }

            if(torque>0){
            // --- preferencia ---
                if (area_m2 > 5.0) {
                    // 5 m² felett → 20Nm motor preferált
                    if (torque >= 20)
                        bd.motorPref += 300;
                    // else
                    //     bd.motorPref += 100;
                }
                else {
                    // 5 m² alatt → 10Nm motor preferált
                    if (torque < 20)
                        bd.motorPref += 300;
                }
            }
        }

        // ------------------------------------------------------------
        // ⭐ 2) KÉSZLET PREFERENCIA — minden anyagra
        // ------------------------------------------------------------
        // auto stockEntries = StockRegistry::instance().findByMaterialId(mat->id);

        // bool inStock = false;
        // for (const auto& se : stockEntries) {
        //     if (se.quantity > 0) {
        //         inStock = true;
        //         break;
        //     }
        // }

        // if (inStock)
        //     bd.stockPref += 200;
        // else{
        //     bd.stockPref -= 200;
        //     r.bomWarnings[id] << "Nincs készleten";
        // }

        // ------------------------------------------------------------
        // ⭐ 2) KÉSZLET PREFERENCIA — bundle robbantás + komponens ellenőrzés
        // ------------------------------------------------------------

        // Stock komponensekre robbantva
        QMap<QUuid,int> stock = StockRegistry::instance().readAllAggregated();

        // BOM-anyag komponensekre robbantva
        QMap<QUuid,int> need;

        if (mat->kind == MaterialKind::Simple) {
            need[mat->id] += 1;
        }
        else if (mat->kind == MaterialKind::Bundle) {
            auto comps = BundleRegistry::instance().componentsOf(mat->bundleCode);
            for (const auto& c : comps)
                need[c.materialId] += c.count;
        }

        // Ellenőrzés
        bool ok = true;

        for (auto compId : need.keys()) {

            int required = need[compId];
            int available = stock.value(compId, 0);

            if (available < required) {
                ok = false;

                const MaterialMaster* cm = MaterialRegistry::instance().findById(compId);
                QString cname = cm ? cm->name : "ismeretlen";

                r.bomWarnings[id] << QString(
                                         "%1 hiányzik (need=%2, stock=%3)"
                                         ).arg(cname)
                                         .arg(required)
                                         .arg(available);
            }
        }

        if (ok)
            bd.stockPref += 200;
        else
            bd.stockPref -= 300;



        // kiértékelés

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

    // qDebug() << "MaterialSelector ranked result:";
    // for (const auto& c : ranked) {
    //     const MaterialMaster* m = MaterialRegistry::instance().findById(c.id);
    //     if (!m) continue;

    //     qDebug().nospace()
    //         << "  total=" << c.totalScore()
    //         << "  [exact=" << c.breakdown.colorExact
    //         << ", nat=" << c.breakdown.colorNat
    //         << ", light=" << c.breakdown.colorLightness
    //         << ", penalty=" << c.breakdown.colorPenalty
    //         << ", axis=" << c.breakdown.axisPref
    //         << ", motor=" << c.breakdown.motorPref
    //         << ", stock=" << c.breakdown.stockPref
    //         << "]"
    //         << "  barcode=" << m->barcode
    //         << "  name=" << m->name
    //         << "  color=" << m->color.code()
    //         << "  diameter=" << m->diameter_mm
    //         << "  stockLength=" << m->stockLength_mm;

    // }

    r.ranked = result;
    return r;
}

// QUuid MaterialSelector::selectPreferred(
//     const QVector<QUuid>& bomList,
//     const Cutting::Plan::Request& req)
// {
//     auto ranked = rankMaterials(bomList, req);
//     return ranked.ranked.isEmpty() ? QUuid() : ranked.ranked.first();
// }
QUuid MaterialSelector::selectPreferred(
    const QVector<QUuid>& bomList,
    const Cutting::Plan::Request& req)
{
    MaterialSelectionResult res = rankMaterials(bomList, req);

    // 1) ranked sorrendben megyünk
    for (const QUuid& id : res.ranked) {

        // 2) ha nincs warning → ez a legjobb
        if (!res.materialWarnings.contains(id))
            return id;
    }

    // 3) ha minden warningos → ranked első elem fallback
    return res.ranked.isEmpty() ? QUuid() : res.ranked.first();
}

