#pragma once

/**
 * @brief Oszlopindexek a CuttingPlan eredménytáblához (tableResults).
 */

namespace ResultsTableColumns {

enum Column {
    RodId = 0,       ///< Emberi azonosító (pl. Rod-A1)
    Barcode,         ///< Forrás barcode (MAT-xxxx vagy RST-xxxx)
    Category,        ///< Anyag + csoport badge
    Length,          ///< Teljes hossz (mm)
    Kerf,            ///< Összes kerf (mm)
    Waste            ///< Hulladék (mm)
};

}
