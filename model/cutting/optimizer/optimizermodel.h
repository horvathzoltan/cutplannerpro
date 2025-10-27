#pragma once

#include <QObject>
#include <QVector>
#include <QMap>
#include <QSet>

#include <model/cutting/piece/piecewithmaterial.h>
#include "../plan/cutplan.h"
#include "../result/resultmodel.h"
#include "../plan/request.h"
#include "../../leftoverstockentry.h"
//#include "../../stockentry.h"
#include "model/cutting/cuttingmachine.h"
#include "model/inventorysnapshot.h"

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
enum class TargetHeuristic {
    ByCount,       // 📊 Legtöbb darab
    ByTotalLength  // 📏 Legnagyobb összhossz
};

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

private:

/**
 * @brief Egy kiválasztott rúd adatait tartalmazza, amiből vágni fogunk.
 *
 * Ez a segédstruktúra arra szolgál, hogy egy helyen legyen minden fontos adat
 * a következő feldolgozandó rúdról. Így nem kell külön változókból összeszedegetni.
 *
 * Mit tartalmaz?
 * - materialId: az anyag azonosítója (milyen anyagból van a rúd)
 * - length: a rúd teljes hossza milliméterben
 * - isReusable: igaz/hamis jelző, hogy ez egy újrafelhasznált hulló-e (true),
 *   vagy egy új rúd a készletből (false)
 * - barcode: a rúd vonalkódja vagy egyedi azonosítója
 *
 * Röviden: ez a "kiválasztott rúd csomag", amit az optimalizáló éppen feldarabol.
 */
    struct SelectedRod {
        QUuid materialId;
        int length = 0;
        bool isReusable = false;
        QString barcode;   // külső címke (RST-xxxx)
        QString rodId;     // belső identitás (ROD-xxxx)
        std::optional<QUuid> entryId; // csak ha reusable
    };

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
    int planCounter = 0; // 🔢 Globális batch számláló
    //QSet<QString> _usedLeftoverBarcodes; // ♻️ már felhasznált hullók nyilvántartása

    QMap<QUuid, QString> leftoverRodMap;   // entryId → rodId
    QSet<QUuid> _usedLeftoverEntryIds; // ♻️ már felhasznált leftover entryId-k



private:
    QVector<LeftoverStockEntry> _localLeftovers;  // csak az aktuális optimize futás idejére


    // QVector<LeftoverStockEntry> allReusableLeftovers() const {
    //     QVector<LeftoverStockEntry> all = _inventorySnapshot.reusableInventory;
    //     all += _localLeftovers;
    //     return all;
    // }

    //     void consumeLeftover(const LeftoverStockEntry& stock);

    /**
 * @brief Megkeresi a legjobb darabkombinációt egy adott rúdhoz.
 *
 * Mit csinál?
 * - Kap egy listát a rendelkezésre álló darabokról (ugyanabból az anyagcsoportból),
 * - Kap egy hosszkorlátot (a rúd teljes hossza),
 * - Kap egy kerf értéket (a vágások miatti veszteség).
 *
 * Ezután végigpróbálja a darabok különböző kombinációit, és kiválasztja azt,
 * amelyik a legjobban kihasználja a rudat:
 * - minél több darabot sikerül kivágni,
 * - minél kevesebb hulladék marad.
 *
 * Röviden: ez a "kombináció-kereső", ami megmondja,
 * hogy mely darabokat érdemes együtt kivágni egy rúdból.
 */
    QVector<Cutting::Piece::PieceWithMaterial> findBestFit(
        const QVector<Cutting::Piece::PieceWithMaterial>& available,
        int lengthLimit,
        double kerf_mm) const;

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
        int indexInView;                  // az összefésült nézet indexe (csak kereséshez)
        LeftoverStockEntry stock;         // a kiválasztott leftover
        QVector<Cutting::Piece::PieceWithMaterial> combo;
        int waste;

        enum class Source { GlobalSnapshot, LocalPool } source; // ⬅ forrás megjelölése
    };


    /**
 * @brief Megkeresi, hogy a hulló készletből (reusableInventory) van-e olyan darab,
 *        amiből érdemes újra vágni.
 *
 * Mit csinál?
 * - Végignézi az összes hulló rudat (reusableInventory),
 * - Megnézi, hogy az adott anyagcsoport darabjai közül melyek férnek bele,
 * - Kiszámolja, mennyi darabot lehet belőle kivágni és mennyi hulladék marad,
 * - Kiválasztja a legjobb találatot (ahol a legtöbb darabot sikerül elhelyezni,
 *   és a legkevesebb a veszteség).
 *
 * Ha talál ilyet, visszaad egy ReusableCandidate-et, ami tartalmazza:
 * - melyik hulló rúd volt az,
 * - milyen darabokat sikerült belőle kivágni,
 * - mennyi hulladék maradt.
 *
 * Ha nem talál megfelelő hullót, akkor üres (std::nullopt) az eredmény.
 *
 * Röviden: ez a "hulló-vadász", ami megmondja,
 * hogy érdemes-e egy meglévő maradékból dolgozni, vagy sem.
 */
    std::optional<ReusableCandidate> findBestReusableFit(
        const QVector<LeftoverStockEntry>& reusableInventory,
        int globalCount,
        const QVector<Cutting::Piece::PieceWithMaterial>& pieces,
        QUuid materialId,
        double kerf_mm
        ) const;
    void cutSinglePieceBatch(const Cutting::Piece::PieceWithMaterial &piece,
                             int &remainingLength, const SelectedRod &rod,
                             const CuttingMachine &machine,
                             int currentOpId,
                             int rodId,
                             double kerf_mm,
                             QVector<Cutting::Piece::PieceWithMaterial> &groupVec);
    void cutComboBatch(const QVector<Cutting::Piece::PieceWithMaterial> &combo,
                       int &remainingLength,
                       const SelectedRod &rod,
                       const CuttingMachine &machine,
                       int currentOpId,
                       int rodId,
                       double kerf_mm,
                       QVector<Cutting::Piece::PieceWithMaterial> &groupVec);
};

} //end namespace Optimizer
} //end namespace Cutting
