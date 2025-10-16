#pragma once

#include <QUuid>
#include <QString>
#include <QColor>

#include "material/materialmaster.h"
//#include "model/cutting/result/cutresult.h"
#include "model/cutting/result/leftoversource.h"


/// 🧩 Újrafelhasználható maradék anyag reprezentációja
struct LeftoverStockEntry {
    QUuid entryId = QUuid::createUuid(); // 🔑 automatikus UUID generálás

    QUuid materialId;           // 🔗 Anyag azonosító
    int availableLength_mm;         // 📏 Szálhossz milliméterben
    Cutting::Result::LeftoverSource source = Cutting::Result::LeftoverSource::Manual; // 🔄 Forrás: Manual vagy Optimization
    std::optional<int> optimizationId = std::nullopt; // 🔍 Csak ha forrás Optimization
    QUuid storageId;                // 📦 Tárolási hely azonosítója

    QString barcode; // 🧾 Egyedi azonosító hulladékdarabra

    bool used = false;   // ➕ új mező: jelzi, hogy már felhasználtuk
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
    QString storageName() const;

    bool isReusable() const {
        return !barcode.isEmpty(); // vagy más logika, pl. hossz > 0 && nem selejt
    }

};
