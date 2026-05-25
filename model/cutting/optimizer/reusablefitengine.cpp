#include "fitengine.h"
#include "reusablefitengine.h"
#include "service/cutting/optimizer/optimizerutils.h"
#include "materials/utils/material_group_utils.h"

/*
♻️ A reusable rudak sorbarendezése garantálja, hogy előbb próbáljuk a kisebb, „kockáztathatóbb” rudakat
✂️ A pontszámítás továbbra is érvényes: preferáljuk a több darabot és a kisebb hulladékot
 */
std::optional<ReusableCandidate>
ReusableFitEngine::findBestReusableFit(const QVector<LeftoverStockEntry>& mergedView,
                                       int globalCount,
                                       const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
                                       QUuid materialId,
                                       double kerf_mm,
                                       const QSet<QUuid>& usedLeftoverEntryIds,
                                       Cutting::Optimizer::OptimizerModel& model)
{

    const MaterialMaster* mat = MaterialRegistry::instance().findById(materialId);
    QString mat1 = mat ? mat->name +":"+mat->barcode : materialId.toString();

    zInfo(QString("🔍 HULLÓ KERESÉSE — anyag=%1").arg(mat1));

    if(pieces.size()>0){
        zInfo("pieces:");

        for(const auto& p : pieces){
            const MaterialMaster* mat2 = MaterialRegistry::instance().findById(p.materialId);
            auto a = QString("len=%1 material=%2 extRef=%3")
                      .arg(p.info.length_mm)
                      .arg(mat2?mat2->toDisplay():p.materialId.toString())
                      .arg(p.info.externalReference);

            zInfo("   •"+a);
        }
    } else{
        zInfo("pieces: (empty)");
    }

    if(mergedView.size()>0){
        zInfo("reuseables:");
        for (const auto& e : mergedView) {
            const MaterialMaster* mat2 = MaterialRegistry::instance().findById(e.materialId);
            zInfo(QString("   •material=%1 barcode=%2 len=%3 materialBarcode=%4 used=%5 source=%6")
                      .arg(mat2?mat2->toDisplay():e.materialId.toString())
                      .arg(e.barcode)
                      .arg(e.availableLength_mm)
                      .arg(e.materialBarcode())
                      .arg(e.used)
                      .arg(e.sourceAsString()));
        }
    } else{
        zInfo("reuseables: (empty)");
    }

    std::optional<ReusableCandidate> best;
    int bestScore = std::numeric_limits<int>::min();

    // ha csoporttag, a materialid, akkor a csoport összes material tagját adja
    // ha nem csoporttag, akkor egy önmagát tartalmazó csoportot adunk viassza
    QSet<QUuid> groupedMaterialIds = GroupUtils::groupMembers(materialId);

    QVector<int> _aff_limits;
    QVector<int> _aff_results;


    // 🔎 releváns darabok kiszűrése
    // itt azt keressük, hogy a paraméterben megkapott materialId-hez anyagcsoportjához
    // melyik piecesek tartoznak  - tehát itt a materialId szerinti anyagcsoport szerint szűrünk
    QVector<Cutting::Piece::PieceWithMaterial> relevantPieces;
    for (const auto& p : pieces)
    {
        if (groupedMaterialIds.contains(p.materialId))
            relevantPieces.append(p);
    }

    int smallestPending = std::numeric_limits<int>::max();
    for (const auto& p : pieces)
        smallestPending = std::min(smallestPending, p.info.length_mm);
    if (smallestPending == std::numeric_limits<int>::max())
        smallestPending = 0;

    int minUsefulLimit = smallestPending > 0
                             ? smallestPending + OptimizerUtils::roundKerfLoss(1, kerf_mm)
                             : 0;

    zInfo("LEFTOVER LOOP START");
    for (int i = 0; i < mergedView.size(); ++i) {
        const auto& stock = mergedView[i];

        zInfo(QString("🔎 Vizsgálat: leftover[%1] — hossz=%2 mm").arg(i).arg(stock.availableLength_mm));

        if (stock.used) {
            zInfo("   ✖ Elutasítva — már felhasznált leftover");
            continue;
        }
        if (!groupedMaterialIds.contains(stock.materialId)) {
            zInfo("   ✖ Elutasítva — rossz anyagcsoport");
            continue;
        }
        if (usedLeftoverEntryIds.contains(stock.entryId)) {
            zInfo("   ✖ Elutasítva — már tiltott leftover");
            continue;
        }

        if (stock.availableLength_mm < OptimizerConstants::MINIMUM_HULLO_MM) {
            zInfo(QString("   ✖ Elutasítva — leftover túl rövid a biztonságos vágáshoz (min=%1 mm)")
                      .arg(OptimizerConstants::MINIMUM_HULLO_MM));
            continue;
        }

        int minSingleFit = smallestPending > 0
                               ? smallestPending + OptimizerUtils::roundKerfLoss(1, kerf_mm)
                               : 0;
        if (minSingleFit > 0 && stock.availableLength_mm < minSingleFit) {
            zInfo(QString("   ✖ Elutasítva — leftover nem tud befogadni egyetlen darabot sem (minSingleFit=%1 mm)")
                      .arg(minSingleFit));
            continue;
        }

        if (stock.availableLength_mm < minUsefulLimit) {
            zInfo("   ✖ Elutasítva — leftover túl rövid bármely pending darabhoz");
            continue;
        }
        // PRIORITÁS: egy darab, ami pontosan elfogyasztja
        const auto single = OptimizerUtils::findSingleExactFit(relevantPieces, stock.availableLength_mm, kerf_mm);
        if (single.has_value()) {
            int kerfTotal = OptimizerUtils::roundKerfLoss(1, kerf_mm);
            int used = single->info.length_mm + kerfTotal;
            int waste = OptimizerUtils::computeWasteInt(stock.availableLength_mm, used);
            if (waste == 0) {
                return ReusableCandidate{ i, stock, QVector<Cutting::Piece::PieceWithMaterial>{ *single }, waste, ReusableCandidate::Source::GlobalSnapshot };
            }
        }

        // Egyébként: keresd a legjobb részhalmazt
        FitEngine::FitResult fit =
            FitEngine::findBestFit(relevantPieces, stock.availableLength_mm, kerf_mm);

        model._fitTelemetry.accumulate(fit);

        zInfo(QString("    strategy=%1, picks=%2, waste=%3")
                  .arg(fit.strategyString())
                  .arg(fit.pieceCount)
                  .arg(fit.waste));

        if (fit.combo.isEmpty()) {
            zInfo("    result: FAILED");
            continue;
        }

        zInfo("   ✔ Találat — combo sikeres");
        // 🔍 LOG: FitEngine combo tartalma
        zInfo("    combo:");
        for (const auto& cp : fit.combo) {
            const MaterialMaster* m = MaterialRegistry::instance().findById(cp.materialId);
            //QString mat2 = m ? m->name + ":" + m->barcode : cp.materialId.toString();

            zInfo(QString("        • len=%1  material=%2  extRef=%3")
                      .arg(cp.info.length_mm)
                      .arg(m?m->toDisplay():cp.materialId.toString())
                      .arg(cp.info.externalReference));
        }


        _aff_limits.append(stock.availableLength_mm);
        _aff_results.append(fit.pieceCount);

        if (fit.combo.isEmpty()) continue;

        // A FitResult már tartalmazza:
        int used           = fit.used;
        int waste          = fit.waste;
        int leftoverLength = stock.availableLength_mm - used;

        if (used > stock.availableLength_mm) continue;


        int score = OptimizerUtils::calcScore(fit.combo.size(), waste, leftoverLength);

        if (score > bestScore) {
            zInfo("   ➡ Új legjobb jelölt kiválasztva");

            bestScore = score;
            ReusableCandidate cand;
            cand.indexInView = i;
            cand.stock       = stock;
            cand.combo       = fit.combo;
            cand.waste       = waste;
            cand.source      = (i < globalCount)
                              ? ReusableCandidate::Source::GlobalSnapshot
                              : ReusableCandidate::Source::LocalPool;
            best = cand;
        }

        zInfo(QString("    scoring: pieces=%1 waste=%2 leftover=%3 → score=%4, bestScore=%5")
                  .arg(fit.combo.size())
                  .arg(waste)
                  .arg(leftoverLength)
                  .arg(score)
                  .arg(bestScore));

    }

    zInfo("📊 HULLÓ KERESÉS LEZÁRVA");

    return best;
}

