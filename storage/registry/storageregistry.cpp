#include "storageregistry.h"

#include <QRegularExpression>

StorageRegistry& StorageRegistry::instance() {
    static StorageRegistry reg;
    return reg;
}

void StorageRegistry::setData(const QVector<StorageEntry>& data) {
    _data = data;
    _uniqueNameCache.clear();   // ⭐ invalidate cache
}

void StorageRegistry::clearAll() {
    _data.clear();
    _uniqueNameCache.clear();   // ⭐ invalidate cache
}

const StorageEntry* StorageRegistry::findById(const QUuid& id) const {
    for (const auto& s : _data) {
        if (s.id == id)
            return &s;
    }
    return nullptr;
}

QVector<StorageEntry> StorageRegistry::findByParentId(const QUuid& parentId) const {
    QVector<StorageEntry> result;
    for (const auto& s : _data) {
        if (s.parentId == parentId)
            result.append(s);
    }
    return result;
}

const StorageEntry* StorageRegistry::findByBarcode(const QString& barcode) const {
    for (const auto& s : _data) {
        if (s.barcode == barcode)
            return &s;
    }
    return nullptr;
}

// storageregistry.cpp
QStringList StorageRegistry::getNamesRecursive(const QUuid& rootId) const {

    auto a = getRecursive(rootId);
    QStringList result;

    for (const auto& entry : a) {
        result.append(entry.name);
    }
    return result;
}

QVector<StorageEntry> StorageRegistry::getRecursive(const QUuid& rootId) const {
    QVector<StorageEntry> result;

    if (auto rootOpt = findById(rootId)) {
        result.append(*rootOpt);
    }

    collectChildrenRecursive(rootId, result);
    return result;
}

void StorageRegistry::collectChildrenRecursive(const QUuid& parentId, QVector<StorageEntry>& out) const {
    const auto& children = findByParentId(parentId);
    for (const auto& child : children) {
        out.append(child);
        collectChildrenRecursive(child.id, out);
    }
}

bool StorageRegistry::isDescendantOf(const QUuid& childId, const QUuid& ancestorId) const {
    auto current = findById(childId);
    while (current) {
        if (current->parentId == ancestorId)
            return true;
        current = findById(current->parentId);
    }
    return false;
}

QString StorageRegistry::uniqueHumanName(const QUuid& id) const
{
    // 1) Cache hit
    if (_uniqueNameCache.contains(id))
        return _uniqueNameCache[id];

    const StorageEntry* s = findById(id);
    if (!s) return "—";

    int depth = 1;

    while (true) {
        QString candidate = buildCandidate(s, depth);

        if (isUniqueCandidate(candidate, id, depth)){
            _uniqueNameCache[id] = candidate;   // ⭐ cache store
            return candidate;
            }

        depth++;

        // fallback: teljes path
        if (depth > 10){   // soha nem lesz ilyen mély
            _uniqueNameCache[id] = candidate;   // ⭐ cache store
            return candidate;
            }
    }
}

bool StorageRegistry::isUniqueCandidate(const QString& candidate, const QUuid& selfId, int depth) const
{
    int count = 0;

    for (const auto& s : _data) {
        QString cand = buildCandidate(&s, depth);
        if (cand == candidate && s.id != selfId)
            count++;
    }

    return count == 0;
}


QString StorageRegistry::buildCandidate(const StorageEntry* s, int depth) const
{
    QStringList parts;
    const StorageEntry* cur = s;

    for (int i = 0; i < depth && cur; ++i) {
        parts.prepend(cur->name);
        cur = findById(cur->parentId);
    }

    return parts.join(" / ");
}


QStringList StorageRegistry::collectBarcodeSegments(const StorageEntry* s) const
{
    QStringList segments;

    const StorageEntry* cur = s;
    while (cur) {

        QString typeName;
        switch (cur->type.value) {
        case StorageType::Type::Warehouse: typeName = "warehouse"; break;
        case StorageType::Type::Rack:      typeName = "rack"; break;
        case StorageType::Type::Shelf:     typeName = "shelf"; break;
        case StorageType::Type::Box:       typeName = "box"; break;
        case StorageType::Type::Pallet:    typeName = "pallet"; break;
        default:                           typeName = "other"; break;
        }

        QString name = cur->name.trimmed();
        if (name.isEmpty())
            name = cur->barcode;   // ⭐ fallback: barcode mindig egyedi

        QString seg = typeName + "_" + name;

        segments.prepend(seg);
        cur = findById(cur->parentId);
    }

    return segments;
}


QString StorageRegistry::shortenSegment(const QString& segment) const
{
    QString s = segment.trimmed().toLower();

    // 1) prefix + suffix szétbontása
    QStringList parts = s.split('_', Qt::SkipEmptyParts);
    QString type = parts.value(0);
    QString name = parts.value(1);

    // 2) type → prefix
    QString prefix;

    if (type == "warehouse") prefix = "w";
    else if (type == "rack") prefix = "r";
    else if (type == "shelf") prefix = "p";     // polc
    else if (type == "box") prefix = "b";
    else if (type == "pallet") prefix = "pl";
    else if (type == "other") prefix = "o";
    else prefix = type.left(1);

    // 3) name → suffix (régi logika alkalmazva)
    QString suffix;

    // 20J → 20j
    QRegularExpression reWarehouse("^([0-9]+[a-z]?)$");
    auto w = reWarehouse.match(name);
    if (w.hasMatch())
        suffix = w.captured(1).toLower();

    // Vasudvar → vas
    else if (name.contains("vas"))
        suffix = "vas";

    // ÚjCsarnok → ucs
    else if (name.contains("csarnok"))
        suffix = "ucs";

    // Roletta → rol
    else if (name.contains("roletta"))
        suffix = "rol";

    // Virtuális → virt
    else if (name.contains("virtu"))
        suffix = "virt";

    // Jobb állvány → j
    else if (name.contains("állvány") && name.contains("jobb"))
        suffix = "j";

    // Bal állvány → b
    else if (name.contains("állvány") && name.contains("bal"))
        suffix = "b";

    // Első_1 → 1
    else {
        QRegularExpression reRack("^első[_ ]?(\\d+)$");
        auto r1 = reRack.match(name);
        if (r1.hasMatch())
            suffix = r1.captured(1);
    }

    // Hátsó_2 → 2
    if (suffix.isEmpty()) {
        QRegularExpression reRack2("^hátsó[_ ]?(\\d+)$");
        auto r2 = reRack2.match(name);
        if (r2.hasMatch())
            suffix = r2.captured(1);
    }

    // Polc 3 → 3
    if (suffix.isEmpty()) {
        QRegularExpression rePolc("^polc[_ ]?(\\d+)$");
        auto p = rePolc.match(name);
        if (p.hasMatch())
            suffix = p.captured(1);
    }

    // Hulló J → hj
    if (suffix.isEmpty()) {
        if (name.contains("hulló") && name.contains("j"))
            suffix = "hj";
        else if (name.contains("hulló") && name.contains("b"))
            suffix = "hb";
    }

    // fallback
    if (suffix.isEmpty())
        suffix = name.left(3);

    // 4) prefix + suffix összeépítése
    return prefix + suffix;
}

QString StorageRegistry::shortenFurther(const QString& code) const
{
    QStringList parts = code.split('-', Qt::SkipEmptyParts);

    // 1) Ha van hosszú szegmens → azt rövidítjük
    for (int i = 0; i < parts.size(); ++i) {
        QString& p = parts[i];

        // ha 4+ karakter → rövidítjük 3-ra
        if (p.length() > 3) {
            p = p.left(3);
            return parts.join("-");
        }
    }

    // 2) Ha minden szegmens 3 vagy kevesebb → rövidítjük 2-re
    for (int i = 0; i < parts.size(); ++i) {
        QString& p = parts[i];

        if (p.length() > 2) {
            p = p.left(2);
            return parts.join("-");
        }
    }

    // 3) Ha minden szegmens 2 vagy kevesebb → rövidítjük 1-re
    for (int i = 0; i < parts.size(); ++i) {
        QString& p = parts[i];

        if (p.length() > 1) {
            p = p.left(1);
            return parts.join("-");
        }
    }

    // 4) Ha már minden szegmens 1 karakter → utolsó fallback
    // hozzáadunk egy számlálót (deterministic suffix)
    return code + "x";
}

// QString StorageRegistry::generateLogisticBarcode(const StorageEntry* s)
// {
//     QStringList segments = collectBarcodeSegments(s);

//     QStringList shortParts;
//     for (const QString& seg : segments)
//         shortParts << shortenSegment(seg);

//     QString candidate = shortParts.join("-");

//     // ütközésmentesítés
//     while (_usedLogisticCodes.contains(candidate)) {
//         candidate = shortenFurther(candidate);
//     }

//     _usedLogisticCodes.insert(candidate);
//     return candidate;
// }

QString StorageRegistry::generateLogisticBarcode(const StorageEntry* s)
{
    // 1) összegyűjtjük a path barcode-okat
    QStringList barcodes;
    const StorageEntry* cur = s;
    while (cur) {
        barcodes.prepend(cur->barcode);
        cur = findById(cur->parentId);
    }

    // 2) leaf barcode
    QString leaf = barcodes.last();

    // 3) parent barcode-ok eldobása
    QString result = leaf;
    for (int i = 0; i < barcodes.size() - 1; ++i) {
        const QString& parent = barcodes[i];
        result.replace(parent, "");
    }

    // 4) normalizálás: elválasztók eltávolítása
    result.replace("__", "_");
    result.replace("_", "");
    result = result.trimmed();

    return result;
}
