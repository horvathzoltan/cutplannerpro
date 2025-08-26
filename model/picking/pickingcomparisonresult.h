#pragma once

#include <QString>
#include "model/picking/pickingitem.h"
#include "model/storageaudit/storageauditentry.h"

struct PickingComparisonResult {
    PickingItem picking;
    StorageAuditEntry audit;
    QString status;             // ✅ "OK", ❌ "Hiány", ⚠️ "Többlet", 🔍 "Nem található"
    int deltaQuantity = 0;      // 📊 Eltérés mértéke
};


/*
if (audit.actualQuantity == picking.requestedQuantity) status = "OK";
else if (audit.actualQuantity < picking.requestedQuantity) status = "Hiány";
else if (audit.actualQuantity > picking.requestedQuantity) status = "Többlet";
else status = "Nem található";

*/

