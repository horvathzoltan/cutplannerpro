#pragma once

#include <QString>
#include "../registries/materialgroupregistry.h"

/*
📦 Three Phase Import
Three Phase Import egy strukturált CSV-adatfeldolgozási minta, amely
három jól elkülönített lépésben alakítja át a nyers sorokat domain objektumokká:

🔁 Fázisok
Convert Phase A nyers CSV sorokat típusos Row struktúrákká alakítjuk.
Példa: MaterialGroupRow, MaterialGroupMemberRow

Build Phase A Row struktúrákból domain objektumokat építünk.
Példa: MaterialGroup, QUuid (anyag ID)

Assemble Phase Az objektumokat összefésüljük, regisztráljuk, vagy más struktúrába illesztjük.
Példa: MaterialGroupRegistry::registerGroup

🎯 Előnyök
Modularitás: minden fázis külön tesztelhető

Hibatűrés: soronkénti validáció és logolás

Olvashatóság: világos adatátalakítási lépések

Skálázhatóság: könnyen alkalmazható más CSV struktúrákra
*/

class MaterialGroupRepository {
public:
        /// Fő belépési pont: betölti a csoportokat és tagokat CSV-ből
    static bool loadFromCsv(MaterialGroupRegistry& registry);
private:
    /// Csoport definíciók egy sora
    struct MaterialGroupRow {
        QString groupKey;
        QString groupName;
        QString colorHex;
    };

    /// Csoport tagság egy sora
    struct MaterialGroupMemberRow {
        QString groupKey;
        QString materialBarCode;
    };


    // --- Stage 1: Convert ---
    static std::optional<MaterialGroupRow> convertRowToMaterialGroupRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<MaterialGroupMemberRow> convertRowToMaterialGroupMemberRow(const QVector<QString>& parts, int lineIndex);

    // --- Stage 2: Build ---
    static std::optional<MaterialGroup> buildMaterialGroupFromRow(const MaterialGroupRow& row, int lineIndex);
    static std::optional<QUuid> buildMaterialIdFromMemberRow(const MaterialGroupMemberRow& row, int lineIndex);

    // --- Stage 3: Load & Assemble ---
    static QVector<MaterialGroupRow> loadGroupRows(const QString& filepath);
    static QVector<MaterialGroupMemberRow> loadMemberRows(const QString& filepath);
    static void addMaterialToGroup(MaterialGroup* group, const QUuid& materialId);
};

