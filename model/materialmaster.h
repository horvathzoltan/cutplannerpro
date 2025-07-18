#pragma once

#include <QString>
#include <QSizeF>
#include "identifiableentity.h"
#include "materialtype.h"
#include "crosssectionshape.h"

enum class ProfileCategory {
    RollerTube,  // Tengelyek (csőmotorhoz)
    BottomBar,   // Súlyprofilok (redőny, napháló alja)
    Unknown
};

// 📦 Anyagdefiníció: szálhossz, forma, méret, szín, típus, súly, gép
struct MaterialMaster : public IdentifiableEntity {
    double stockLength_mm = 0.0;       // 📏 Teljes szálhossz mm-ben (pl. 6000)

    CrossSectionShape shape;           // 🧩 Keresztmetszet formája
    double diameter_mm = 0.0;          // ⚪ Kör formánál: átmérő
    QSizeF size_mm;                    // ▭ Téglalapnál: szélesség és magasság mm-ben

    QString ralColorCode;              // 🎨 Szín RAL-kóddal (pl. "RAL 9006")
    MaterialType type;                 // 🧪 Anyagtípus: Aluminium / Steel / stb.
    ProfileCategory category = ProfileCategory::Unknown; // 🗂️ Gyártási kategória

    double weightPerStock_kg = 0.0;    // ⚖️ Teljes szál súlya kg-ban
    QString defaultMachineId;          // ⚙️ Ajánlott gép az anyaghoz

    QString coating;       // pl. fehér, szürke, szinterezhető
    QString comment;       // opcionális, UI-ba is jó

    MaterialMaster(); // 🔧 Default konstruktor deklaráció

};
