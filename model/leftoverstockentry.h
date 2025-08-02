#pragma once

#include <QUuid>
#include <QString>
#include <QColor>

#include "materialmaster.h"
#include "model/cutresult.h"


/// 🧩 Újrafelhasználható maradék anyag reprezentációja
struct LeftoverStockEntry {
    QUuid entryId = QUuid::createUuid(); // 🔑 automatikus UUID generálás


    QUuid materialId;           // 🔗 Anyag azonosító
    int availableLength_mm;         // 📏 Szálhossz milliméterben
    LeftoverSource source = LeftoverSource::Manual; // 🔄 Forrás: Manual vagy Optimization
    std::optional<int> optimizationId = std::nullopt; // 🔍 Csak ha forrás Optimization

    QString barcode; // 🧾 Egyedi azonosító hulladékdarabra
    /// 🧪 Egyenlőség vizsgálat (opcionális)
    bool operator==(const LeftoverStockEntry& other) const;

    QString materialName() const;  // 📛 Anyag neve
    QString reusableBarcode() const; // 🧾 Saját Vonalkód
    QString materialBarcode() const; // 🧾 Material Vonalkód
    MaterialType materialType() const; // 🧬 Anyagtípus
    const MaterialMaster* master() const;

    QString materialGroupName() const;
    QColor materialGroupColor() const;
    QString sourceAsString() const;
};
