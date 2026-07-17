#include "material_selector.h"
#include "materials/registry/material_registry.h"
#include "materials/model/material_family_utils.h"

struct Candidate {
    QUuid id;
    int score;
};

QVector<QUuid> MaterialSelector::rankMaterials(
    const QVector<QUuid>& bomList,
    const Cutting::Plan::Request& req)
{
    QVector<Candidate> ranked;
    // 1) Családonkénti csoportosítás
    QMap<MaterialFamily, QVector<Candidate>> groups;

    for (auto id : bomList) {
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(id);
        if (!mat)
            continue;

        int score = 0;

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

            // ⭐ Festett termék → natúr anyag a legjobb
            if (!matHasColor) {
                score += 200;
            }
            else {
                // ⭐ Festett termék → világosság alapján rangsorolunk
                if (diff < 0.10) {
                    score += 120;   // nagyon hasonló → kiváló alap
                }
                else if (diff < 0.25) {
                    score += 40;    // közepesen hasonló → jó
                }
                else {
                    score -= 80;    // nagyon eltérő → kerülendő
                }
            }
        }
        else
        {
            // ⭐ Natúr termék → natúr anyag enyhén preferált
            if (!matHasColor)
                score += 50;
        }


        // ------------------------------------------------------------
        // ⭐ 2) TENGELY PREFERENCIA (hosszfüggő)
        // ------------------------------------------------------------
        if (mat->family == MaterialFamily::Tengely) {
            int L = req.requiredLength;

            if (L > 3500) {
                if (mat->diameter_mm >= 78)
                    score += 300;
                else
                    score -= 100;
            } else {
                if (mat->diameter_mm == 70)
                    score += 150;
            }
        }

        // ------------------------------------------------------------
        // ⭐ 3) SÚLY PREFERENCIA (pl. 3000 mm előnyben)
        // ------------------------------------------------------------
        if (mat->family == MaterialFamily::Suly) {
            if (mat->stockLength_mm == 3000)
                score += 100;
        }

        // ------------------------------------------------------------
        // ⭐ 4) RAKTÁRKÉSZLET (ha később lesz ilyen)
        // ------------------------------------------------------------
        // if (Inventory::instance().isInStock(mat->id))
        //     score += 50;

        ranked << Candidate{id, score};

        groups[mat->family].append({id, score});
    }

    // ------------------------------------------------------------
    // ⭐ Rangsorolás
    // ------------------------------------------------------------
    // 2) Családon belüli sort
    for (auto& vec : groups) {
        std::sort(vec.begin(), vec.end(),
                  [](const Candidate& a, const Candidate& b){
                      return a.score > b.score;
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
            << "  score=" << c.score
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
