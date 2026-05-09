#pragma once

#include <QObject>
#include <QVector>
#include <QMap>
#include <QSet>

#include "../piece/piecewithmaterial.h"
#include "../plan/cutplan.h"
#include "../result/resultmodel.h"
#include "../plan/request.h"
#include "../../leftoverstockentry.h"
//#include "../../stockentry.h"
#include "../cuttingmachine.h"
#include "../../inventorysnapshot.h"
#include "cuttypes.h"
#include "selectedrod.h"


namespace Cutting {
namespace Optimizer {

/**
 * @brief Beállítja, hogy az optimalizáló hogyan válassza ki a következő feldolgozandó anyagcsoportot.
 *
 * Kétféle "okosság" (heurisztika) közül lehet választani:
 *
 * - ByCount (📊 darabszám alapján):
 *   Mindig azt az anyagcsoportot választja, ahol a legtöbb darab várakozik.
 *   Példa: ha van egy csoportban 12 rövid darab, a másikban csak 2 hosszú,
 *   akkor a 12 darabos csoport kerül előre, mert ott több a feldolgoznivaló.
 *
 * - ByTotalLength (📏 összhossz alapján):
 *   Mindig azt az anyagcsoportot választja, ahol a darabok teljes hossza a legnagyobb.
 *   Példa: ha van egy csoportban 10 db 100 mm-es darab (összesen 1000 mm),
 *   és egy másikban 2 db 800 mm-es darab (összesen 1600 mm),
 *   akkor a 2 darabos csoport kerül előre, mert ott nagyobb a teljes hossz.
 *
 * Röviden:
 * - ByCount = "ahol több a darab"
 * - ByTotalLength = "ahol több az anyag"
 */


class OptimizerModel : public QObject {
    Q_OBJECT

public:
    explicit OptimizerModel(QObject *parent = nullptr);

    /**
 * @brief A teljes vágási tervek listáját adja vissza (referenciaként).
 *
 * Ez tartalmazza az összes elkészült CutPlan-t, vagyis minden egyes rúd teljes vágási tervét.
 * Itt az látszik, hogy melyik rúd hogyan lett feldarabolva, milyen darabok kerültek belőle kivágásra,
 * mennyi volt a kerf (fűrészvágás miatti veszteség), és mennyi hulladék maradt.
 *
 * Röviden: ez a "nagy kép", a teljes vágási folyamat terve rudanként.
 */
    QVector<Cutting::Plan::CutPlan> &getResult_PlansRef();

/**
 * @brief A vágások után keletkezett hulló darabok listáját adja vissza.
 *
 * Ez NEM a teljes terv, hanem csak a maradékok (leftovers) gyűjteménye.
 * Minden elem egy ResultModel, ami tartalmazza:
 * - melyik tervből származik a hulló,
 * - mekkora a hossza,
 * - milyen anyagból van,
 * - újrafelhasználható-e, vagy végső hulladék.
 *
 * Röviden: ez a "melléktermék lista", vagyis mi maradt a vágások után.
 */
    QVector<Cutting::Result::ResultModel> getResults_Leftovers() const;


    /**
 * @brief Az optimalizáló fő folyamata: elkészíti a vágási terveket és a hulló listát.
 *
 * Mit csinál ez a függvény?
 * 1. Összegyűjti a vágandó darabokat, és anyag szerint csoportosítja őket.
 * 2. Amíg van feldolgozatlan darab:
 *    - kiválasztja, melyik anyagcsoporttal foglalkozzon (a beállított heurisztika alapján:
 *      ByCount = ahol több a darab, ByTotalLength = ahol több az anyag),
 *    - megnézi, van-e hozzá megfelelő gép,
 *    - először próbál hullóból (reusable) darabolni,
 *    - ha nincs megfelelő hulló, akkor a készletből (stock) választ rudat,
 *    - ha egyik sem jó, a darabot eldobja.
 * 3. Ha talált megfelelő rudat:
 *    - kivágja belőle a legjobb kombinációt,
 *    - eltávolítja a felhasznált darabokat a listából,
 *    - létrehoz egy CutPlan-t (vágási tervet) a teljes rúdról,
 *    - létrehoz egy ResultModel-t a maradékról (hulló).
 * 4. Minden lépésről audit logot ír (gépadatok, kerf, hulladék).
 * 5. A végén kitakarítja a reusable készletből a már felhasznált hullókat.
 *
 * Röviden: ez a "nagy varázsló", ami a bemeneti igényekből (darabok + készlet)
 * elkészíti a teljes vágási terveket és a hulló listát.
 */
    void optimize(TargetHeuristic heuristic);
    //void optimize_old(TargetHeuristic heuristic);

    void setCuttingRequests(const QVector<Cutting::Plan::Request>& list);

    void setInventorySnapshot(const InventorySnapshot &snapshot){
        _inventorySnapshot = snapshot;
    }

    CutResult cutCombo_WithLifecycle(const QVector<Cutting::Piece::PieceWithMaterial> &combo, int &remainingLength, int &remainingLength2, const SelectedRod &rod, const CuttingMachine &machine, int currentOpId, int rodId, double kerf_mm, QVector<Cutting::Piece::PieceWithMaterial> &groupVec);
    CutResult cutSingle_WithLifecycle(const Cutting::Piece::PieceWithMaterial &piece, int &remainingLength, int &remainingLength2, const SelectedRod &rod, const CuttingMachine &machine, int currentOpId, int rodId, double kerf_mm, QVector<Cutting::Piece::PieceWithMaterial> &groupVec);


    void applyFrontTrimToPlan(const QUuid &planId, double kerf_mm, bool isStockRod);
private:

    // A felhasználótól érkező vágási igények (darabok listája).
    // Ezek a bemeneti adatok, az optimalizáció alatt nem módosulnak.
    QVector<Cutting::Plan::Request> _requests;

    // A készlet pillanatképe (snapshot), amely tartalmazza a teljes rudakat és a maradékokat.
    // Ez egy homokozó másolat, amelyet a registrykből töltünk be.
    // Az optimalizáció ezen dolgozik, a valódi registryket nem érinti.
    InventorySnapshot _inventorySnapshot;

    // Az optimalizáció eredménye: minden egyes rúdhoz létrejött vágási terv.
    QVector<Cutting::Plan::CutPlan> _result_plans;

    // Az optimalizáció során előrejelzett maradékok (planned leftovers).
    // Ezek a "mi lenne, ha" kimenetek, csak finalize után kerülhetnek vissza a registrybe.
    QVector<Cutting::Result::ResultModel> _planned_leftovers;

    // 🔢 Globális számláló minden optimize futáshoz.
    // Minden új optimalizációs futás (optimize()) kap egy egyedi azonosítót,
    // amit a CutPlan-ekhez és ResultModel-ekhez rendelünk, hogy auditban
    // és logban visszakövethető legyen, melyik futásból származnak.
    int nextOptimizationId = 1;

    int planCounter = 0;   // csak CutPlan sorszám
    int rodCounter  = 0;   // csak RodId kiosztás

    //QSet<QString> _usedLeftoverBarcodes; // ♻️ már felhasznált hullók nyilvántartása

    QMap<QUuid, QString> leftoverRodMap;   // entryId → rodId
    QSet<QUuid> _usedLeftoverEntryIds; // ♻️ már felhasznált leftover entryId-k

private:
    QVector<LeftoverStockEntry> _localLeftovers;  // csak az aktuális optimize futás idejére

    void logCutState(const Cutting::Plan::CutPlan &p, int remainingLengthBefore, int remainingLengthAfter);

    void finalizeRod(const SelectedRod& rod,
                     int remainingLength,
                     int currentOpId);

    CutResult applyLifecycle(const CutResult &cr, int &remainingLength, int &remainingLength2, const SelectedRod &rod, int currentOpId, QVector<Cutting::Piece::PieceWithMaterial> &groupVec);
};

} //end namespace Optimizer
} //end namespace Cutting
