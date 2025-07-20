#pragma once

#include <QUuid>

struct CuttingRequest {
    QUuid materialId;       // 🔗 Törzsbeli azonosító
    int requiredLength;     // ✂️ Vágás hossza
    int quantity;           // Hány darab kell

    bool isValid() const;
    QString invalidReason() const;
};

