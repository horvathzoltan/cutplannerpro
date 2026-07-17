#pragma once

#include <QVector>
#include <QUuid>
#include <QString>

#include <materials/model/material_family_detector.h>
//#include <optional>
#include "materials/model/material_master.h"

class MaterialRegistry {
private:
    MaterialRegistry() = default;  // 🔐 Privát konstruktor a singletonhoz
    MaterialRegistry(const MaterialMaster&) = delete;

    QVector<MaterialMaster> _data;  // 📦 Betöltött anyagtörzs lista
public:

    // 🔁 Singleton elérés
    static MaterialRegistry& instance();

    void setData(const QVector<MaterialMaster>& v) { _data = v;}
    // ➕ Új anyag hozzáadása, csak ha code egyedi
    bool registerData(const MaterialMaster& material);

    const QVector<MaterialMaster>& readAll() const { return _data;}
    const MaterialMaster* findById(const QUuid& id) const;
    const MaterialMaster* findByBarcode(const QString& barcode) const;

    bool isBarcodeUnique(const QString& barcode) const;

    bool isEmpty() const { return _data.isEmpty(); }

    // void applyFamilyDetection() {
    //     for (auto& m : _data) {
    //         m.family = MaterialFamilyDetector::detect_fromBarcode(m.barcode);
    //     }
    // }

    QVector<QUuid> generateBom(QUuid typeId, QUuid subtypeId) const;
};
