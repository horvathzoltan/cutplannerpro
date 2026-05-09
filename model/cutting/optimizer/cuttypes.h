#pragma once
#include "model/cutting/plan/cutplan.h"
#include "model/cutting/result/resultmodel.h"
#include "model/leftoverstockentry.h"
#include <QUuid>

enum class CutResultStatus {
    Ok,
    Overfill,
    Unknown
};

struct CutResult {
    CutResultStatus status;
    QUuid planId;
    int used;
    int waste;

    Cutting::Plan::CutPlan plan;
    Cutting::Result::ResultModel result;
    QVector<QUuid> usedPieceIds;  // ← OptimizerModel törli majd a darabokat
};


/**
 * @brief Egy újrafelhasználható (hulló) rúd legjobb találatát írja le.
 *
 * Amikor az optimalizáló megpróbál darabokat elhelyezni egy már meglévő hulló rúdban,
 * akkor a keresés eredményét ebben a struktúrában adjuk vissza.
 *
 * Mit tartalmaz?
 * - indexInInventory: a reusableInventory-ban hol található ez a hulló darab
 * - stock: maga a hulló rúd leírása (anyag, hossz, azonosító, stb.)
 * - combo: azok a darabok, amiket ebből a hullóból sikerült kivágni
 * - totalWaste: mennyi maradék (hulladék) maradt a vágás után
 *
 * Röviden: ez a "nyertes ajánlat" egy hulló rúdból.
 * Ha találunk ilyet, akkor ebből vágunk, nem veszünk elő új rudat a készletből.
 */

struct ReusableCandidate {
    int indexInView;
    LeftoverStockEntry stock;
    QVector<Cutting::Piece::PieceWithMaterial> combo;
    int waste;

    enum class Source { GlobalSnapshot, LocalPool } source;
};
