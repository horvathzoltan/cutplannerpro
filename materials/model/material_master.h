#pragma once

#include <QString>
#include <QSizeF>
#include "model/identifiableentity.h"
#include "common/color/namedcolor.h"
#include "materials/model/material_type.h"
#include "model/crosssectionshape.h"
#include "materials/model/cutting_mode.h"
#include "materials/model/painting_mode.h"
#include "scoringparams.h"
#include "trimmingparams.h"

// 📦 Anyagdefiníció: szálhossz, forma, méret, szín, típus, súly, gép
struct MaterialMaster : public IdentifiableEntity {
    MaterialMaster(){}; // 🔧 Default konstruktor deklaráció

    double stockLength_mm = 0.0;       // 📏 Teljes szálhossz mm-ben (pl. 6000)

    CrossSectionShape shape;           // 🧩 Keresztmetszet formája
    double diameter_mm = 0.0;          // ⚪ Kör formánál: átmérő
    QSizeF size_mm;                    // ▭ Téglalapnál: szélesség és magasság mm-ben

    //QString ralColorCode;              // 🎨 Szín RAL-kóddal (pl. "RAL 9006")
    MaterialType type;                 // 🧪 Anyagtípus: Aluminium / Steel / stb.
    //ProfileCategory category = ProfileCategory::Unknown; // 🗂️ Gyártási kategória

    double weightPerStock_kg = 0.0;    // ⚖️ Teljes szál súlya kg-ban
    QString defaultMachineId;          // ⚙️ Ajánlott gép az anyaghoz

    //QString coating;       // pl. fehér, szürke, szinterezhető
    NamedColor color; // 🎨 Anyag színe (RAL vagy HEX kód)

    PaintingMode paintingMode = PaintingMode::Paintable; // jelzi, ha az anyag festhető
    CuttingMode cuttingMode = CuttingMode::Length; // 🔧 Alapértelmezés: szálhossz vágás

    QString comment;       // opcionális, UI-ba is jó

    int trim_mm = 15;          // technikai vágás eleje/vége
    int minLeftOver_mm = 70;       // minimális maradék (usable leftover)
    int scrap_mm = 300;        // ez alatt selejt

    int goodLeftOver_Min_mm = 500;  // jó leftover tartomány alsó határa
    int goodLeftOver_Max_mm = 800;  // jó leftover tartomány felső határa

    QString externalCode;

    MaterialScoringParams scoringParams() const {
        return {
            scrap_mm,
            goodLeftOver_Min_mm,
            goodLeftOver_Max_mm
        };
    }

    MaterialTrimmingParams trimmingParams(bool isReusable) const {
        return {
            isReusable ? 0 : trim_mm, // frontTrim
            trim_mm,                  // backTrim
            minLeftOver_mm            // minHull
        };
    }

};
