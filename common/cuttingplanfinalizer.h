#pragma once  // 👑 Modern include guard — helyettesíti a klasszikus #ifndef/#define-et

#include <QVector> // Qt-típus a vágási tervek és hulladéklista kezelésére

// 🔽 Model osztályok, amikkel dolgozunk
#include "../model/cutresult.h"              // Vágás eredménye — tartalmaz hulladék adatokat
#include "../model/cutplan.h"                // Egyedi vágási terv — anyag, hossz, reusable info

// 🔽 Készletregiszterek — stock és reusable kezelése
#include "../model/registries/stockregistry.h"
#include "../model/registries/reusablestockregistry.h"

// 🔽 Hulladék reprezentáció
#include "../model/reusablestockentry.h"

// 🔽 Konverziós logika: CutResult → ReusableStockEntry
#include "../common/cutresultutils.h"

/**
 * @brief Statikus finalizer osztály, ami lezárja a vágási tervet.
 * Feladatai:
 *  - Levonás a felhasznált készletekből
 *  - Új hulladékdarabok regisztrálása
 *  - Tervek státuszának lezárása (logikailag a Presenter végzi)
 */
class CuttingPlanFinalizer
{
public:
    /**
     * @brief Finalizálja a megadott vágási terveket és hulladékokat.
     *
     * @param plans A vágási tervek listája
     * @param leftovers A vágások során keletkezett hulladékdarabok
     */
    static void finalize(QVector<CutPlan>& plans,
                         const QVector<CutResult>& leftovers);
};

