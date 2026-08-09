#pragma once

#include <QString>
#include <QStringList>

namespace Cutting {
namespace SnapshotFormat {

/**
 * @brief A snapshot verziószáma.
 * Minden formátumváltozásnál növelni kell.
 */
static constexpr int SNAPSHOT_VERSION = 2;

/**
 * @brief A snapshot szekciók nevei.
 * Ezeket a Serializer írja ki, a Deserializer pedig felismeri.
 */
static const QString SECTION_VERSION   = "#VERSION";
static const QString SECTION_CUTPLANS  = "#CUTPLANS";
static const QString SECTION_SEGMENTS  = "#SEGMENTS";
static const QString SECTION_PIECES    = "#PIECES";
static const QString SECTION_LEFTOVERS = "#LEFTOVERS";

/**
 * @brief A CUTPLANS sor mezőinek sorrendje (CSV)
 *
 * planId;
 * planNumber;
 * statusCsv;
 * rodId;
 * materialBarcode;
 * machineBarcode;
 * machineKerf;
 * sourceCsv;
 * sourceBarcode;
 * optimizationId;
 * toldasRoleCsv;
 * parentCsv
 */
enum CutPlanField {
    CP_PLAN_ID = 0,
    CP_PLAN_NUMBER,
    CP_STATUS,
    CP_ROD_ID,
    CP_MATERIAL_BARCODE,
    CP_MACHINE_BARCODE,
    CP_MACHINE_KERF,
    CP_SOURCE,
    CP_SOURCE_BARCODE,
    CP_OPTIMIZATION_ID,
    CP_TOLDAS_ROLE,
    CP_PARENT_CSV
};

/**
 * @brief A SEGMENTS sor mezőinek sorrendje (CSV)
 *
 * planId;
 * segIndex;
 * typePrefix;
 * length_mm;
 * barcode;
 * pieceId;
 * externalRef
 */
enum SegmentField {
    SG_PLAN_ID = 0,
    SG_INDEX,
    SG_TYPE,
    SG_LENGTH,
    SG_BARCODE,
    SG_PIECE_ID,
    SG_EXTERNAL_REF
};

/**
 * @brief A PIECES sor mezőinek sorrendje (CSV)
 *
 * planId;
 * pieceId;
 * length_mm;
 * materialBarcode;
 * externalRef
 */
enum PieceField {
    PC_PLAN_ID = 0,
    PC_PIECE_ID,
    PC_LENGTH,
    PC_MATERIAL_BARCODE,
    PC_EXTERNAL_REF
};

/**
 * @brief A LEFTOVERS sor mezőinek sorrendje (CSV)
 *
 * planId;
 * leftoverBarcode;
 * waste_mm
 */
enum LeftoverField {
    LO_PLAN_ID = 0,
    LO_BARCODE,
    LO_WASTE
};

} // namespace SnapshotFormat
} // namespace Cutting
