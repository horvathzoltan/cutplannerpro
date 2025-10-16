#pragma once

#include <QString>

/**
 * @brief Darabolási szakasz típusa — a rúd struktúrájához
 */


/**
 * @brief Egy szakasz a vágási tervben (darab, kerf, hulladék)
 */

namespace Cutting{
namespace Segment{

struct SegmentModel {

    enum class Type {
        Piece,   // ✂️ Kért darab
        Kerf,    // ⚙️ Vágási veszteség
        Waste    // 🪓 Végmaradék / selejt
    };

    double length_mm;
    Type type;

    /**
     * @brief Szöveges leírás a típushoz (exporthoz / UI-hoz)
     */
    QString typeAsString() const {
        switch (type) {
        case Type::Piece:  return "Piece";
        case Type::Kerf:   return "Kerf";
        case Type::Waste:  return "Waste";
        }
        return "Unknown";
    }

    /**
     * @brief Rövid string a munkalaphoz (pl. [1800], [K3], [W194])
     */
    QString toLabelString() const {
        switch (type) {
        case Type::Piece: return QString("[%1]").arg(length_mm);
        case Type::Kerf:  return QString("[K%1]").arg(length_mm);
        case Type::Waste: return QString("[W%1]").arg(length_mm);
        }
        return QString("[?%1]").arg(length_mm);
    }

    QVector<SegmentModel> generateSegments(double kerf_mm, double totalLength_mm) const;
};
}} //end namespace Cutting::Segment
