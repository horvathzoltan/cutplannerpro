#pragma once

namespace OptimizerConstants {
constexpr int SELEJT_THRESHOLD = 300;     // ez alatt minden selejt
constexpr int GOOD_LEFTOVER_MIN = 500;    // jó leftover alsó határa
constexpr int GOOD_LEFTOVER_MAX = 800;    // jó leftover felső határa
inline constexpr int MinPieceLength = SELEJT_THRESHOLD; // legkisebb darabhossz
inline constexpr int END_TRIM_MM = 15;      // gyári sérült vég
inline constexpr int MINIMUM_HULLO_MM = 70; // gépészeti minimális maradék
}
