#pragma once

#include <QVector>
#include <QUuid>
#include <QMap>
#include <QSet>
#include <QRegularExpression>
//#include <optional>
#include "storage/model/storageentry.h"

class StorageRegistry {
private:
    enum class TagCategory {
        Parent,
        Leaf,
        Position,
        Side,          // régi: J, B, RJ, RB
        Identifier,
        WarehouseSide, // ÚJ: R, L (raktár térfél oldaliság)
        RackSide,      // ÚJ: RR, RL, LR, LL (rack oldaliság)
        RackPosition,  // ÚJ: R1, R2, L1, L2 (rack pozíciók)
        Other
    };


    struct SemanticRule {
        QString name;
        QRegularExpression pattern;
        QVector<StorageType::Type> allowedTypes;
    };

    QVector<SemanticRule> _semanticRules;


    StorageRegistry();
    QVector<StorageEntry> _data;
    QMap<QUuid, QString> _uniqueNameCache;
    QMap<QUuid, QString> _logisticBarcodes;
    bool _logisticInitialized = false;
    mutable QSet<QString> _semanticFixes;
    QUuid _fallbackId;

    void collectChildrenRecursive(const QUuid& parentId, QStringList& out) const;
    bool isUniqueCandidate(const QString& candidate, const QUuid& selfId, int depth) const;
    QString buildCandidate(const StorageEntry *s, int depth) const;

    QStringList splitBarcode(const QString &barcode) const;
    QStringList reduceTags(const QStringList &parentTags, const QStringList &childTags) const;
    QString generateLogisticBarcode(const StorageEntry *s);

    QStringList normalizeTags(const QStringList &tags) const;
    QStringList reduceChild(const QStringList &parent, const QStringList &child) const;
    bool fuzzyPrefixMatch(const QStringList &parent, const QStringList &child) const;
    bool isLeafTag(const QString &tag) const;
    void validateRepeatedTags(const StorageEntry *s, const QStringList &tags) const;
    void validateSimilarTags(const StorageEntry *s, const QStringList &tags) const;
    StorageRegistry::TagCategory categorizeTag(const QString &tag) const;
    void initializeSemanticRules();
    void validateSemantic(const StorageEntry *s) const;
    //QString suggestSemanticFix(const StorageEntry *s) const;
    QString suggestWarehouseFix(const QString &bc) const;
    QString suggestRackFix(const QString &bc) const;
    QString suggestShelfFix(const QString &bc) const;
    QString suggestFloorFix(const QString &bc) const;
    QString suggestCrateFix(const QString &bc) const;
    QString suggestZoneFix(const QString &bc) const;
    bool isBarcodeUnique(const QString &bc, const QUuid &selfId) const;
    bool isFixUnique(const QString &bc, const QUuid &selfId) const;
    QString makeFixUnique(const QString &bc, const QUuid &selfId) const;
public:
    static StorageRegistry& instance();

    // 📥 Betöltés kívülről (pl. repositoryból)
    void setData(const QVector<StorageEntry>& data);

    // 📤 Elérés
    QVector<StorageEntry> readAll() const { return _data; }

    // 🔍 Keresés egyedi azonosítóval
    const StorageEntry* findById(const QUuid& id) const;

    // 🔍 Keresés parentId alapján (fa-nézethez)
    QVector<StorageEntry> findByParentId(const QUuid& parentId) const;

    // 🔄 Teljes törlés (UI reset esetén pl.)
    void clearAll();
    const StorageEntry* findByBarcode(const QString &barcode) const;

    // 🆕 Root + children (rekurzív) lekérdezés
    QStringList getNamesRecursive(const QUuid& rootId) const;
    QVector<StorageEntry> getRecursive(const QUuid &rootId) const;
    void collectChildrenRecursive(const QUuid &parentId, QVector<StorageEntry> &out) const;
    bool isDescendantOf(const QUuid &childId, const QUuid &ancestorId) const;
    QString uniqueHumanName(const QUuid &id) const;
    //bool isUnique(const QString &name);

    void initializeLogisticBarcodes();
    QString logisticBarcode(const QUuid &id) const;
    void validateTagDifferences(const StorageEntry *s, const QStringList &tags) const;
    void validateBarcode(const StorageEntry *s) const;

    const StorageEntry* fallbackStorage() const { return findById(_fallbackId); }
};
