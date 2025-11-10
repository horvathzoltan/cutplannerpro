#pragma once

/**
 * @brief Oszlopindexek a CuttingPlan eredménytáblához (tableResults).
 */

namespace ResultsTableColumns {

enum Column {
    RodId = 0,       ///< Emberi azonosító (pl. Rod-A1)
    Material,        ///< Anyag + csoport badge + forrás barcode
    Length,          ///< Teljes hossz (mm)
    Kerf,            ///< Összes kerf (mm)
    Waste            ///< Hulladék (mm)
};

}
