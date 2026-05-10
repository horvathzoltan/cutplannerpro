#pragma once

#include <QUuid>
#include <QString>
#include <QColor>

#include "common/logger.h"
#include "materials/model/material_master.h"
//#include "model/cutting/result/cutresult.h"
#include "cutting/result/leftoversource.h"
#include <QDebug>

/// 🧩 Újrafelhasználható maradék anyag reprezentációja
struct LeftoverStockEntry {
    QUuid entryId;

    // Csak itt generálódjon új GUID
    LeftoverStockEntry() : entryId(QUuid::createUuid()) {
        zInfo("CTOR new entryId=" + entryId.toString());
    }

    // Másoláskor és assignmentnél megtartja az eredeti entryId-t
    LeftoverStockEntry(const LeftoverStockEntry& other) = default;
    LeftoverStockEntry& operator=(const LeftoverStockEntry& other) = default;

    QUuid materialId;           // 🔗 Anyag azonosító
    int availableLength_mm;         // 📏 Szálhossz milliméterben
    Cutting::Result::LeftoverSource source = Cutting::Result::LeftoverSource::Manual; // 🔄 Forrás: Manual vagy Optimization
    std::optional<int> optimizationId = std::nullopt; // 🔍 Csak ha forrás Optimization
    QUuid storageId;                // 📦 Tárolási hely azonosítója

    QString barcode; // 🧾 Egyedi azonosító hulladékdarabra
    bool used = false;

    // 🔗 Új mező: a közvetlen forrás hulló azonosítója
    std::optional<QString> parentBarcode;

    /// 🧪 Egyenlőség vizsgálat (opcionális)
    bool operator==(const LeftoverStockEntry& other) const;

    QString materialName() const;  // 📛 Anyag neve
    //QString reusableBarcode() const; // 🧾 Saját Vonalkód
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

    QString toLeftoverEvent(QString rodId);
};
