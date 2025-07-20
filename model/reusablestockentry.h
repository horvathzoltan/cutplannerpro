#pragma once

#include <QUuid>
#include <QString>
#include <QColor>

#include "materialmaster.h"
#include "model/cutresult.h"


/// 🧩 Újrafelhasználható maradék anyag reprezentációja
struct ReusableStockEntry {
    QUuid materialId;           // 🔗 Anyag azonosító
    int availableLength_mm;         // 📏 Szálhossz milliméterben
    LeftoverSource source = LeftoverSource::Manual; // 🔄 Forrás: Manual vagy Optimization
    std::optional<int> optimizationId = std::nullopt; // 🔍 Csak ha forrás Optimization

    /// 🧪 Egyenlőség vizsgálat (opcionális)
    bool operator==(const ReusableStockEntry& other) const;

    QString name() const;  // 📛 Anyag neve
    QString barcode() const; // 🧾 Vonalkód
    MaterialType type() const; // 🧬 Anyagtípus
    const MaterialMaster* master() const;



    QString groupName() const;
    QColor groupColor() const;
    QString sourceAsString() const;
};
