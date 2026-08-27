#include "storageregistry.h"
#include "common/logger.h"
#include "common/stringsimilarity_helper.h"

#include <QRegularExpression>

StorageRegistry::StorageRegistry()
{
   // initializeSemanticRules();   // ⭐ új
}

StorageRegistry& StorageRegistry::instance() {
    static StorageRegistry reg;
    return reg;
}

void StorageRegistry::setData(const QVector<StorageEntry>& data) {
    _data = data;

    // teljes reset
    _uniqueNameCache.clear();
    _logisticBarcodes.clear();
    _semanticFixes.clear();
    _logisticInitialized = false;

    // FALLBACK ellenőrzése
    const StorageEntry* fb = findByBarcode("FALLBACK");
    if (!fb) {
        zError("❌ A FALLBACK tároló hiányzik a storage.csv-ből!");
        abort();
    }
    _fallbackId = fb->id;

    // szemantikai validáció + logisztikai címkék generálása
    initializeLogisticBarcodes();

    // ⭐ Logisztikai címkék listázása induláskor
    // const auto& allStorages = StorageRegistry::instance().readAll();
    // zInfo("=== Storage logisztikai címkék: ===");
    // for (const auto& s : allStorages) {
    //     QString logCode = StorageRegistry::instance().logisticBarcode(s.id);
    //     zInfo(
    //         QString("%5 Storage: %1 | Barcode: %2 | Logistic: %3")
    //             .arg(s.name)
    //             .arg(s.barcode)
    //             .arg(logCode).arg(s.type.icon())
    //         );
    // }
    // zInfo("=== === ===");
}

void StorageRegistry::clearAll() {
    _data.clear();
    _uniqueNameCache.clear();
    _logisticBarcodes.clear();
    _semanticFixes.clear();
    _logisticInitialized = false;
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

/*logisztikai barcode generálás*/

QStringList StorageRegistry::splitBarcode(const QString& barcode) const
{
    if (barcode.contains('_'))
        return barcode.split('_', Qt::SkipEmptyParts);

    return { barcode };
}

QStringList StorageRegistry::reduceTags(const QStringList& parentTags,
                                        const QStringList& childTags) const
{
    if (childTags.size() >= parentTags.size()) {
        bool match = true;
        for (int i = 0; i < parentTags.size(); ++i) {
            if (childTags[i] != parentTags[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            return childTags.mid(parentTags.size());
        }
    }

    return childTags;
}


QStringList StorageRegistry::normalizeTags(const QStringList& tags) const
{
    QStringList out;
    QSet<QString> seen;

    for (const QString& t : tags) {
        if (!seen.contains(t)) {
            out.append(t);
            seen.insert(t);
        }
    }
    return out;
}

QStringList StorageRegistry::reduceChild(const QStringList& parent,
                                         const QStringList& child) const
{
    if (fuzzyPrefixMatch(parent, child)) {
        return child.mid(parent.size());
    }

    return child;
}


bool StorageRegistry::fuzzyPrefixMatch(const QStringList& parent, const QStringList& child) const
{
    if (child.size() <= parent.size())
        return false;

    for (int i = 0; i < parent.size(); ++i) {

        QString p = parent[i];
        QString c = child[i];

        // pontos egyezés
        if (p == c)
            continue;

        // fuzzy egyezés: egyik tartalmazza a másikat
        if (c.contains(p) || p.contains(c))
            continue;

        return false;
    }

    return true;
}

QString StorageRegistry::generateLogisticBarcode(const StorageEntry* s)
{
    if (_logisticInitialized) {
        qWarning() << "❌ generateLogisticBarcode() tiltott inicializálás után!";
        return QString();
    }

    // 1) path barcode-ok (root → leaf)
    QList<const StorageEntry*> path;
    const StorageEntry* cur = s;
    while (cur) {
        path.prepend(cur);
        cur = findById(cur->parentId);
    }

    // 2) taglisták összeépítése redundancia nélkül
    QStringList finalTags;

    QStringList parentTags;

    for (const StorageEntry* node : path) {

        QStringList childTags = splitBarcode(node->barcode);

        // prefix redundancia eltávolítása
        QStringList reduced = reduceChild(parentTags, childTags);

        // globális redundancia eltávolítása
        reduced = normalizeTags(reduced);

        finalTags += reduced;

        parentTags = childTags;
    }

    // 3) végső logisztikai címke
    return finalTags.join("-");
}

void StorageRegistry::initializeLogisticBarcodes()
{
    if (_logisticInitialized) {
        qWarning() << "❌ generateLogisticBarcode() hívás tiltott: a logisztikai címkék már inicializálva vannak!";
    }

    // ⭐ 1) Storage audit (elírások, hasonlóságok, különbségek)
    for (const auto& s : _data) {
        validateBarcode(&s);
    }

    for (const auto& s : _data) {
        QString code = generateLogisticBarcode(&s);
        _logisticBarcodes.insert(s.id, code);
    }

    _logisticInitialized = true;
}

QString StorageRegistry::logisticBarcode(const QUuid& id) const
{
    if (!_logisticInitialized) {
        qWarning() << "❌ logisticBarcode() hívás tiltott: a logisztikai címkék még nincsenek inicializálva!";
        return QString();
    }

    return _logisticBarcodes.value(id);
}

bool StorageRegistry::isLeafTag(const QString& tag) const
{
    // Polcok: P1, P2, P3...
    QRegularExpression polc("^P\\d+$");
    if (polc.match(tag).hasMatch())
        return true;

    // Doboz / raklap / egyéb leaf kódok
    // Például: E1_P3 → leaf
    QRegularExpression leaf("^[A-Z]+\\d+_P\\d+$");
    if (leaf.match(tag).hasMatch())
        return true;

    // Ha a storage type leaf → a barcode utolsó tagja leaf
    // (Ez a legpontosabb)
    return false;
}

void StorageRegistry::validateTagDifferences(const StorageEntry* s,
                                             const QStringList& tags) const
{
    for (int i = 0; i < tags.size(); ++i) {
        for (int j = i + 1; j < tags.size(); ++j) {

            QString a = tags[i];
            QString b = tags[j];

            TagCategory ca = categorizeTag(a);
            TagCategory cb = categorizeTag(b);

            // WarehouseSide ↔ RackSide → nem összehasonlítható
            if ((ca == TagCategory::WarehouseSide && cb == TagCategory::RackSide) ||
                (ca == TagCategory::RackSide && cb == TagCategory::WarehouseSide))
                continue;

            // WarehouseSide ↔ RackPosition → nem összehasonlítható
            if ((ca == TagCategory::WarehouseSide && cb == TagCategory::RackPosition) ||
                (ca == TagCategory::RackPosition && cb == TagCategory::WarehouseSide))
                continue;

            // RackSide ↔ RackPosition → nem összehasonlítható
            if ((ca == TagCategory::RackSide && cb == TagCategory::RackPosition) ||
                (ca == TagCategory::RackPosition && cb == TagCategory::RackSide))
                continue;

            // Parent-child tiltás
            if (ca == TagCategory::Parent || cb == TagCategory::Parent)
                continue;

            // Csak azonos kategóriák hasonlíthatók
            if (ca != cb)
                continue;

            // Leaf tagok → nem gyanús
            if (ca == TagCategory::Leaf)
                continue;

            // Pozíció tagok → nem gyanús
            if (ca == TagCategory::Position)
                continue;

            // Oldal tagok → nem gyanús
            if (ca == TagCategory::Side)
                continue;

            // TÚL KÜLÖNBÖZŐ → gyanús
            if (StringSimilarity::fuzzyDifferent(a, b)) {
                qWarning() << "⚠️ Gyanúsan különböző storage tag:"
                           << s->name
                           << "| " << a << "<->" << b;
            }
        }
    }
}



void StorageRegistry::validateSimilarTags(const StorageEntry* s,
                                          const QStringList& tags) const
{
    for (int i = 0; i < tags.size(); ++i) {
        for (int j = i + 1; j < tags.size(); ++j) {

            QString a = tags[i];
            QString b = tags[j];

            TagCategory ca = categorizeTag(a);
            TagCategory cb = categorizeTag(b);

            // WarehouseSide ↔ RackSide → nem összehasonlítható
            if ((ca == TagCategory::WarehouseSide && cb == TagCategory::RackSide) ||
                (ca == TagCategory::RackSide && cb == TagCategory::WarehouseSide))
                continue;

            // WarehouseSide ↔ RackPosition → nem összehasonlítható
            if ((ca == TagCategory::WarehouseSide && cb == TagCategory::RackPosition) ||
                (ca == TagCategory::RackPosition && cb == TagCategory::WarehouseSide))
                continue;

            // RackSide ↔ RackPosition → nem összehasonlítható
            if ((ca == TagCategory::RackSide && cb == TagCategory::RackPosition) ||
                (ca == TagCategory::RackPosition && cb == TagCategory::RackSide))
                continue;


            // Parent-child tiltás
            if (ca == TagCategory::Parent || cb == TagCategory::Parent)
                continue;

            // Csak azonos kategóriák hasonlíthatók
            if (ca != cb)
                continue;

            // Leaf tagok → nem gyanús
            if (ca == TagCategory::Leaf)
                continue;

            // Pozíció tagok → nem gyanús
            if (ca == TagCategory::Position)
                continue;

            // Oldal tagok → nem gyanús
            if (ca == TagCategory::Side)
                continue;

            // túl hasonló → gyanús
            if (StringSimilarity::tooSimilar(a, b)) {
                qWarning() << "⚠️ Gyanúsan hasonló storage tagok:"
                           << s->name
                           << "| Barcode:" << s->barcode
                           << "| Tag1:" << a
                           << "| Tag2:" << b;
            }
        }
    }
}


void StorageRegistry::validateRepeatedTags(const StorageEntry* s,
                                           const QStringList& tags) const
{
    QHash<QString, int> counts;
    for (const QString& t : tags)
        counts[t]++;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {

        const QString& tag = it.key();
        int count = it.value();

        // csak rövid tagokra figyelmeztetünk (pl. B, J, R, E1, H1)
        if (tag.size() <= 3 && count > 1) {
            qWarning() << "⚠️ Ismétlődő rövid storage tag:"
                       << s->name
                       << "| Barcode:" << s->barcode
                       << "| Tag:" << tag
                       << "| Count:" << count
                       << "| Megjegyzés: lehet, hogy a kód nem elég egyértelmű.";
        }
    }
}


void StorageRegistry::validateBarcode(const StorageEntry* s) const
{
    QStringList tags = splitBarcode(s->barcode);

    validateSemantic(s);          // ⭐ új szemantikai validáció

    validateRepeatedTags(s, tags);
    validateSimilarTags(s, tags);
    validateTagDifferences(s, tags);   // ⭐ új
}

StorageRegistry::TagCategory StorageRegistry::categorizeTag(const QString& tag) const
{
    // 1) Raktár oldaliság (Warehouse side)
    if (tag == "R" || tag == "L")
        return TagCategory::WarehouseSide;

    // 2) Rack oldaliság (Rack side)
    QRegularExpression rackSide("^(RR|RL|LR|LL)$");
    if (rackSide.match(tag).hasMatch())
        return TagCategory::RackSide;

    // 3) Rack pozíció (Rack position)
    QRegularExpression rackPos("^[RL]\\d+$");
    if (rackPos.match(tag).hasMatch())
        return TagCategory::RackPosition;

    // 4) Parent tag: hosszú, betű-szám keverék, nem leaf
    if (tag.size() > 3 && !tag.contains('_'))
        return TagCategory::Parent;

    // 5) Leaf tag: P1, P2, P3...
    QRegularExpression leaf("^P\\d+$");
    if (leaf.match(tag).hasMatch())
        return TagCategory::Leaf;

    // 6) Pozíció tag: E1, E2, H1, H2
    QRegularExpression pos("^[EH]\\d+$");
    if (pos.match(tag).hasMatch())
        return TagCategory::Position;

    // 7) Régi oldal tagok (RJ, RB, J, B)
    QRegularExpression side("^(J|B|RJ|RB)$");
    if (side.match(tag).hasMatch())
        return TagCategory::Side;

    // 8) Azonosító tag: S1, A2, stb.
    QRegularExpression id("^[A-Z]\\d+$");
    if (id.match(tag).hasMatch())
        return TagCategory::Identifier;

    return TagCategory::Other;
}


QRegularExpression StorageRegistry::semanticRuleFor(StorageType::Type t) const
{
    if (t == StorageType::Type::Site || t == StorageType::Type::Warehouse)
        return QRegularExpression("^" + PREFIX + POSTFIX.value(t));

    QString prefixOpt = "(" + PREFIX + ")?";
    QString postfix   = POSTFIX.value(t);

    return QRegularExpression("^" + prefixOpt + postfix);
}


void StorageRegistry::validateSemantic(const StorageEntry* s) const
{
    // FALLBACK → speciális tároló, nem validáljuk
    if (s->barcode == "FALLBACK")
        return;

    const QString& bc = s->barcode;
    StorageType::Type t = s->type.value;

    QRegularExpression rule = semanticRuleFor(t);

    if (rule.match(bc).hasMatch()) {
        if (isFixUnique(bc, s->id))
            return;

        QString uniqueFix = makeFixUnique(bc, s->id);
        _semanticFixes.insert(uniqueFix);

        qWarning() << "⚠️ Szemantikai hiba: a barcode nem egyedi!"
                   << s->name
                   << "| Barcode:" << bc
                   << "| Type:" << s->type.toString()
                   << "| Egyedi javítás:" << uniqueFix;
        return;
    }

    // --- 2) JAVÍTÁS PRÓBÁLÁSA ---
    QString fix;

    switch (t) {
    case StorageType::Type::Warehouse: fix = suggestWarehouseFix(bc); break;
    case StorageType::Type::Rack:      fix = suggestRackFix(bc); break;
    case StorageType::Type::Shelf:     fix = suggestShelfFix(bc); break;
    case StorageType::Type::Floor:     fix = suggestFloorFix(bc); break;
    case StorageType::Type::Zone:      fix = suggestZoneFix(bc); break;
    case StorageType::Type::Crate:     fix = suggestCrateFix(bc); break;
    default:                           fix = QString(); break;
    }

    // --- 3) JAVÍTÁS REGEXP ELLENŐRZÉSE ---
    bool fixMatches = false;
    if (!fix.isEmpty()) {
        QRegularExpression fixRule = semanticRuleFor(t);
        fixMatches = fixRule.match(fix).hasMatch();
    }

    if (!fixMatches) {
        // A javítás regexp szerint sem jó → hibát dobunk
        qWarning() << "⚠️ Szemantikai hiba: a barcode nem felel meg a storage típus szabályainak:"
                   << s->name
                   << "| Barcode:" << bc
                   << "| Type:" << s->type.toString()
                   << "| Javaslat nem generálható.";
        return;
    }

    // --- 4) JAVÍTÁS EGYEDISÉG ELLENŐRZÉSE ---
    QString uniqueFix = makeFixUnique(fix, s->id);
    _semanticFixes.insert(uniqueFix);

    // --- 5) HA A JAVÍTÁS == EREDETI ÉS EGYEDI → OK ---
    if (uniqueFix == bc)
        return;

    // --- 6) JAVÍTÁS ≠ EREDETI → HIBA ---
    qWarning() << "⚠️ Szemantikai hiba: a barcode nem felel meg a storage típus szabályainak:"
               << s->name
               << "| Barcode:" << bc
               << "| Type:" << s->type.toString()
               << "| Javasolt javítás:" << uniqueFix;
}


QString StorageRegistry::suggestWarehouseFix(const QString& bc) const
{
    QStringList parts = bc.split('_');
    QString prefix = parts[0];
    QStringList out;
    out << prefix;

    bool firstB = true;

    for (int i = 1; i < parts.size(); ++i) {
        QString p = parts[i];

        if (p == "K") out << "OUT";        // Külső
        else if (p == "B") {
            if (firstB) out << "IN";       // Belső
            else out << "L";               // Bal
            firstB = false;
        }
        else if (p == "J") out << "R";     // Jobb
        else if (p.startsWith("E")) out << "F" + p.mid(1);  // Első → F
        else if (p.startsWith("H")) out << "B" + p.mid(1);  // Hátsó → B
        else out << p;
    }

    return out.join("_");
}


QString StorageRegistry::suggestRackFix(const QString& bc) const
{
    if (bc.contains("_R") || bc.contains("RJ"))
        return "R";

    if (bc.contains("_L") || bc.contains("RB"))
        return "L";

    return "R"; // fallback
}


QString StorageRegistry::suggestShelfFix(const QString& bc) const
{
    QRegularExpression num("(\\d+)$");
    auto m = num.match(bc);
    if (m.hasMatch()) {
        return "P" + m.captured(1);
    }
    return "P1";
}

QString StorageRegistry::suggestFloorFix(const QString& bc) const
{
    // 1) Warehouse-szemantika újrafelhasználása
    QString wh = suggestWarehouseFix(bc);

    // 2) Ha már jó Floor formátum → nem kell javítani
    QRegularExpression good("^[A-Z0-9]{2,5}(_(IN|OUT|R|L|F\\d+|B\\d+))*$");
    if (good.match(wh).hasMatch())
        return wh;

    // 3) fallback: prefix + IN_R
    QString prefix = bc.split('_').first();
    return prefix + "_IN_R";
}


QString StorageRegistry::suggestCrateFix(const QString& bc) const
{
    // Ha már jó (C12, CRATE5, CAGE3, KAL01)
    QRegularExpression good("^(C\\d+|(CRATE|CAGE|KAL)\\d+)$");
    if (good.match(bc).hasMatch())
        return bc;

    // Ha szám van benne → konvertálható
    QRegularExpression num("(\\d+)$");
    auto m = num.match(bc);
    if (m.hasMatch()) {
        return "C" + m.captured(1);   // alapértelmezett crate kód
    }

    // fallback
    return "C1";
}

QString StorageRegistry::suggestZoneFix(const QString& bc) const
{
    QString wh = suggestWarehouseFix(bc);

    // Várjuk: PREFIX_IN_R vagy PREFIX_OUT_L
    QRegularExpression zone("^[A-Z0-9]{2,5}_(IN|OUT)_(R|L)$");
    if (zone.match(wh).hasMatch())
        return wh;

    // Ha túl részletes (pl. VAS_IN_R_F1), akkor csak az első 3 tagot tartjuk meg
    QStringList parts = wh.split('_');
    if (parts.size() >= 3) {
        return parts[0] + "_" + parts[1] + "_" + parts[2];
    }

    // fallback: prefix + IN_R
    QString prefix = bc.split('_').first();
    return prefix + "_IN_R";
}

bool StorageRegistry::isBarcodeUnique(const QString& bc, const QUuid& selfId) const
{
    for (const auto& s : _data) {
        if (s.barcode == bc && s.id != selfId)
            return false;
    }
    return true;
}

bool StorageRegistry::isFixUnique(const QString& bc, const QUuid& selfId) const
{
    // 1) meglévő barcode-ok
    for (const auto& s : _data) {
        if (s.barcode == bc && s.id != selfId)
            return false;
    }

    // 2) korábban generált javítások
    if (_semanticFixes.contains(bc))
        return false;

    return true;
}

QString StorageRegistry::makeFixUnique(const QString& bc, const QUuid& selfId) const
{
    if (isFixUnique(bc, selfId))
        return bc;

    int counter = 1;
    while (true) {
        QString candidate = bc + "_" + QString::number(counter);
        if (isFixUnique(candidate, selfId))
            return candidate;
        counter++;
    }
}

const StorageEntry* StorageRegistry::findByLogisticBarcode(const QString& code) const
{
    if (!_logisticInitialized)
        return nullptr;

    for (auto it = _logisticBarcodes.begin(); it != _logisticBarcodes.end(); ++it) {
        if (it.value().compare(code.trimmed(), Qt::CaseInsensitive) == 0) {
            return findById(it.key());
        }
    }
    return nullptr;
}

