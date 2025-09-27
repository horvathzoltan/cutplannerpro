#pragma once

namespace AuditTableColumns {
static constexpr int Material   = 0;  // Anyag neve + szín (csoport színkód alapján)
static constexpr int Barcode    = 1;  // Vonalkód (csak megjelenítés, nem interaktív)
static constexpr int Storage    = 2;  // Tároló neve (ahonnan vagy ahová az anyag kerül)
static constexpr int Expected   = 3;  // Elvárt mennyiség (összesített vagy egyedi)
static constexpr int Actual     = 4;  // Tényleges mennyiség (interaktív: SpinBox vagy rádiógomb)
static constexpr int Missing    = 5;  // Hiányzó mennyiség (Expected - Actual)
static constexpr int Status     = 6;  // Audit státusz (színezett cella: OK, Missing, Pending)
}
