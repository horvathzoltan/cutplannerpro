#pragma once
#include <QString>

struct PickingItem {
    QString materialName;       // 📛 Anyag neve
    QString materialBarcode;    // 🧾 Vonalkód
    QString colorCode;          // 🎨 Szín
    int requestedQuantity = 0;  // 📦 Igényelt mennyiség (vágási terv alapján)
    QString storageHint;        // 🗂️ Javasolt tárhely (opcionális)
};

