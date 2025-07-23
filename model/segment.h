#pragma once

#include <QString>

/**
 * @brief Darabolási szakasz típusa — a rúd struktúrájához
 */
enum class SegmentType {
    Piece,   // ✂️ Kért darab
    Kerf,    // ⚙️ Vágási veszteség
    Waste    // 🪓 Végmaradék / selejt
};

/**
 * @brief Egy szakasz a vágási tervben (darab, kerf, hulladék)
 */
struct Segment {
    int length_mm;
    SegmentType type;

    /**
     * @brief Szöveges leírás a típushoz (exporthoz / UI-hoz)
     */
    QString typeAsString() const {
        switch (type) {
        case SegmentType::Piece:  return "Piece";
        case SegmentType::Kerf:   return "Kerf";
        case SegmentType::Waste:  return "Waste";
        }
        return "Unknown";
    }

    /**
     * @brief Rövid string a munkalaphoz (pl. [1800], [K3], [W194])
     */
    QString toLabelString() const {
        switch (type) {
        case SegmentType::Piece: return QString("[%1]").arg(length_mm);
        case SegmentType::Kerf:  return QString("[K%1]").arg(length_mm);
        case SegmentType::Waste: return QString("[W%1]").arg(length_mm);
        }
        return QString("[?%1]").arg(length_mm);
    }

    QVector<Segment> generateSegments(int kerf_mm, int totalLength_mm) const;
};
