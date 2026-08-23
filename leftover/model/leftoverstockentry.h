#pragma once

#include <QUuid>
#include <QString>
#include <QColor>
#include <QDateTime>

#include "materialbundles/model/bundle_componentlength.h"
#include "materials/model/material_master.h"
#include "model/cutting/result/leftoversource.h"
#include "leftover/leftoverstatus.h"
#include "model/cutting/plan/parentinfo.h"

/// 🧩 Újrafelhasználható maradék anyag reprezentációja
struct LeftoverStockEntry {

    // Csak itt generálódjon új GUID
    LeftoverStockEntry() : entryId(QUuid::createUuid()) {}
    // Másoláskor és assignmentnél megtartja az eredeti entryId-t
    LeftoverStockEntry(const LeftoverStockEntry& other) = default;
    LeftoverStockEntry& operator=(const LeftoverStockEntry& other) = default;

    QUuid entryId;
    QUuid materialId;           // 🔗 Anyag azonosító
    int availableLength_mm;     // 📏 Szálhossz milliméterben
    Cutting::Result::LeftoverSource source = Cutting::Result::LeftoverSource::Manual; // 🔄 Forrás: Manual vagy Optimization
    std::optional<int> optimizationId = std::nullopt; // 🔍 Csak ha forrás Optimization
    QUuid storageId;                // 📦 Tárolási hely azonosítója

    QString barcode; // 🧾 Egyedi azonosító hulladékdarabra
    //bool used = false;

    // 🔗 Új mező: a közvetlen forrás hulló azonosítója
    //std::optional<QString> parentBarcode;
    std::optional<Cutting::Plan::ParentInfo> _parent = std::nullopt;

    QDateTime createdAt = QDateTime::currentDateTime();
    QDateTime lastSeenAt = QDateTime::currentDateTime();
    LeftoverStatus status = LeftoverStatus::Unknown;

    int notFoundCount = 0;   // 🔍 Audit: hányszor nem találtuk meg

    /// 🧪 Egyenlőség vizsgálat (opcionális)
    bool operator==(const LeftoverStockEntry& other) const;

    QVector<BundleComponentLength> bundleComponentLengths;

    int getComponentLength(QUuid compMatId) const
    {
        for (const auto& c : bundleComponentLengths)
            if (c.materialId == compMatId)
                return c.length_mm == -1 ? availableLength_mm : c.length_mm;

        // ha nincs a listában → nem bundle leftover → fallback
        return availableLength_mm;
    }

    //QString materialName() const;  // 📛 Anyag neve
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
