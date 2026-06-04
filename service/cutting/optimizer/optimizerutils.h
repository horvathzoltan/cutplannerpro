#pragma once
#include <QVector>
#include <materials/model/material_master.h>
#include <materials/model/scoringparams.h>
#include <materials/registry/material_registry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include "../../../model/cutting/piece/piecewithmaterial.h"
#include "common/logger.h"
#include "optimizerconstants.h"

namespace OptimizerUtils {

struct PhysicalCutInfo {
    double totalCut = 0.0;
    double kerfTotal = 0.0;
    double used = 0.0;
};

inline PhysicalCutInfo computePhysicalCut(
    const QVector<Cutting::Piece::PieceWithMaterial>& cuts,
    double kerf_mm,
    int totalLength_mm)
{
    PhysicalCutInfo info;

    if (cuts.isEmpty() || kerf_mm <= 0.0)
        return info;

    double used = 0.0;
    int kerfCount = 0;

    for (int i = 0; i < cuts.size(); ++i) {
        used += cuts[i].info.length_mm;

        bool isLast = (i == cuts.size() - 1);

        if (!isLast) {
            // két darab között mindig kell kerf
            kerfCount++;
            used += kerf_mm;
        } else {
            // utolsó darab után csak akkor kell kerf, ha marad fizikai maradék
            if (used < totalLength_mm) {
                kerfCount++;
                used += kerf_mm;
            }
        }
    }

    info.totalCut = used - kerfCount * kerf_mm;
    info.kerfTotal = kerfCount * kerf_mm;
    info.used = used;

    return info;
}


inline int calcScore(int pieceCount, int waste, int leftoverLength, MaterialScoringParams sp) {
    int score = 0;

    // 🎯 Alap: darabszám preferálása
    score += pieceCount * 100;

    // 🧹 Hulladék levonása
    score -= waste;

    // 🥇 Teljes elfogyasztás – pszichológiai bónusz
    if (waste == 0) {
        score += 800;
    }

    // 🎯 Pontos egy darabos illeszkedés – külön jutalom
    if (leftoverLength == 0 && pieceCount == 1) {
        score += 200;
    }

    // 😊 Jó leftover tartomány – „jó érzésű” maradék
    if (leftoverLength >= sp.goodLeftOver_Min_mm &&
        leftoverLength <= sp.goodLeftOver_Max_mm) {
        score += 300;
    }

    // 😬 Selejt leftover – erősebb büntetés
    if (leftoverLength > 0 && leftoverLength < sp.scrap_mm) {
        score -= 300; // visszaállítva az eredeti szigorra
    }

    // 🧱 Túl nagy leftover – dinamikus büntetés
    if (leftoverLength > sp.goodLeftOver_Max_mm) {
        // Alapbüntetés −100, de skálázva a mérettel
        int oversize = leftoverLength - sp.goodLeftOver_Max_mm;
        score -= 100 + oversize / 100;
        // pl. 900 mm leftover → −110, 1500 mm leftover → −1150
    }

    return score;
}



/**
 * @brief Megkeresi azt az egyetlen darabot, amelyik a legjobban illeszkedik a maradék hosszba.
 *
 * A pontozás a calcScore() alapján történik, tehát preferálja a teljes elfogyasztást
 * és a jó leftover méreteket. Ha nincs olyan darab, ami beleférne, std::nullopt‑ot ad vissza.
 *
 * @param available     Az elérhető darabok listája.
 * @param lengthLimit   A rúd maradék hossza.
 * @param kerf_mm       A vágási veszteség (mm).
 * @return std::optional<Cutting::Piece::PieceWithMaterial>
 */
inline std::optional<Cutting::Piece::PieceWithMaterial>
findSingleBestPiece(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                    int lengthLimit,
                    double kerf_mm)
{
    zInfo(QString("🔍 SingleCut keresés indult — candidates=%1, limit=%2 mm, kerf=%3")
              .arg(available.size())
              .arg(lengthLimit)
              .arg(QString::number(kerf_mm, 'f', 2)));

    std::optional<Cutting::Piece::PieceWithMaterial> best;
    int bestScore = std::numeric_limits<int>::min();

    for (const auto& piece : available) {

        // ⭐ Fizikai kerf modell
        //auto info = OptimizerUtils::computePhysicalCut({ piece }, kerf_mm, lengthLimit);
        auto info = OptimizerUtils::computePhysicalCut({ piece }, 0.0, lengthLimit);
        double used = info.used;

        if (used > lengthLimit) {
            zInfo(QString("   ✖ Elutasítva: piece=%1 mm — used=%2 > limit=%3")
                      .arg(piece.info.length_mm)
                      .arg(used)
                      .arg(lengthLimit));
            continue;
        }

        int waste = static_cast<int>(lengthLimit - used);
        int leftoverLength = waste;

        const MaterialMaster* mat = MaterialRegistry::instance().findById(piece.materialId);
        MaterialScoringParams sp = mat ? mat->scoringParams()
                                       : MaterialScoringParams::getDefault();

        int score = OptimizerUtils::calcScore(1, waste, leftoverLength, sp);

        zInfo(QString("   • Vizsgálat: piece=%1 mm → used=%2, waste=%3, leftover=%4, score=%5")
                  .arg(piece.info.length_mm)
                  .arg(used)
                  .arg(waste)
                  .arg(leftoverLength)
                  .arg(score));

        if (score > bestScore) {
            bestScore = score;
            best = piece;
            zInfo(QString("     ✔ Új legjobb jelölt: piece=%1 mm (score=%2)")
                      .arg(piece.info.length_mm)
                      .arg(score));
        }
    }

    if (best.has_value()) {
        zInfo(QString("🎯 SingleCut találat — bestPiece=%1 mm, bestScore=%2")
                  .arg(best->info.length_mm)
                  .arg(bestScore));
    } else {
        zInfo("♻️ SingleCut — nincs egyetlen darab sem, ami beleférne");
    }

    return best;
}


/**
 * @brief Megkeresi azt az egyetlen darabot, amelyik pontosan elfogyasztja a megadott hosszt.
 *
 * Ha találunk olyan darabot, amelyik + kerf pontosan kitölti a rúd hosszát (waste == 0),
 * akkor azt visszaadjuk. Ellenkező esetben std::nullopt.
 *
 * @param available     Az elérhető darabok listája.
 * @param lengthLimit   A rúd teljes hossza.
 * @param kerf_mm       A vágási veszteség (mm).
 * @return std::optional<Cutting::Piece::PieceWithMaterial>
 */
inline std::optional<Cutting::Piece::PieceWithMaterial>
findSingleExactFit(const QVector<Cutting::Piece::PieceWithMaterial>& available,
                   int lengthLimit,
                   double kerf_mm)
{
    zInfo(QString("🔍 SingleExactFit — keresés indítása (n=%1, limit=%2 mm, kerf=%3)")
              .arg(available.size())
              .arg(lengthLimit)
              .arg(kerf_mm));

    for (const auto& piece : available)
    {
        // ⭐ Fizikai kerf modell
        auto info = OptimizerUtils::computePhysicalCut({ piece }, 0.0, lengthLimit);
        double used = info.used;

        zInfo(QString("   • Vizsgálat: piece=%1 mm → used=%2 mm (kerfTotal=%3)")
                  .arg(piece.info.length_mm)
                  .arg(used)
                  .arg(info.kerfTotal));

        // ⭐ Pontos illeszkedés fizikai modell szerint
        if (std::abs(used - lengthLimit) < 0.0001)
        {
            zInfo(QString("🎯 SingleExactFit — pontos illeszkedés: %1 mm (used=%2)")
                      .arg(piece.info.length_mm)
                      .arg(used));
            return piece;
        }

        zInfo("     ✖ Nem pontos illeszkedés");
    }

    zInfo("❌ SingleExactFit — nincs pontos illeszkedés");
    return std::nullopt;
}


} // namespace OptimizerUtils
