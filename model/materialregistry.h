#pragma once

#include <QVector>
#include <QUuid>
#include <QString>
#include <optional>
#include "materialmaster.h"

class MaterialRegistry {
private:
    MaterialRegistry() = default;  // 🔐 Privát konstruktor a singletonhoz

    QVector<MaterialMaster> materials;  // 📦 Betöltött anyagtörzs lista
public:


    // 🔁 Singleton elérés
    static MaterialRegistry& instance() {
        static MaterialRegistry reg;
        return reg;
    }

    // 🔍 Keresés technikai azonosító szerint (id)
    const MaterialMaster* findById(const QUuid& id) const;

    const QVector<MaterialMaster>& all() const { return materials;}

    bool isBarcodeUnique(const QString& barcode) const;

    // 🔍 Keresés vonalkód alapján
    const MaterialMaster* findByBarcode(const QString& barcode) const;

    // ➕ Új anyag hozzáadása, csak ha code egyedi
    bool insert(const MaterialMaster& material);

    // 📥 Betöltés külső adatokból (pl. CSV, JSON után)
    void setMaterials(const QVector<MaterialMaster>& newMaterials);
};
