#include "model/registries/stockregistry.h"
#include <algorithm>
#include <QDebug>

#include <common/filenamehelper.h>
#include <model/repositories/stockrepository.h>
#include "common/scopedperthreadlock.h" // a korábban beillesztett általános wrapper

StockRegistry& StockRegistry::instance() {
    static StockRegistry reg;
    return reg;
}

void StockRegistry::registerEntry(const StockEntry& entry) {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    _data.append(entry);
    persist();
}

void StockRegistry::clearAll() {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    _data.clear();
    persist();
}

void StockRegistry::removeEntry(const QUuid& id) {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    auto it = std::remove_if(_data.begin(), _data.end(),
                             [&](const StockEntry& r) {
                                 return r.entryId == id;
                             });

    if (it != _data.end()) {
        _data.erase(it, _data.end());
        persist();
    }
}

QVector<StockEntry> StockRegistry::findByGroupName(const QString& name) const {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    QVector<StockEntry> result;
    for (const auto& entry : _data) {
        if (entry.materialGroupName() == name)
            result.append(entry);
    }
    return result;
}

void StockRegistry::persist() const {
    const QString path = FileNameHelper::instance().getStockCsvFile();
    if (path.isEmpty()) return;

    // készítünk egy gyors snapshotot lock alatt, majd az I/O-t lock nélkül végezzük
    QVector<StockEntry> snapshot;
    {
        ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
        snapshot = _data; // gyors másolat
    } // lock feloldódik itt

    // lassú fájlírás lock nélkül
    StockRepository::saveToCSV(snapshot, path);
}


void StockRegistry::consumeEntry(const QUuid& materialId)
{
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    for (StockEntry& entry : _data) {
        if (entry.materialId == materialId) {
            if (entry.quantity > 0) {
                entry.quantity -= 1;
                persist();
            }
            break;
        }
    }
}

std::optional<StockEntry> StockRegistry::findById(const QUuid& entryId) const {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    for (const auto& r : _data) {
        if (r.entryId == entryId)
            return r;
    }
    return std::nullopt;
}

bool StockRegistry::updateEntry(const StockEntry& updated) {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    for (auto& r : _data) {
        if (r.entryId == updated.entryId) {
            r = updated;
            persist();
            return true;
        }
    }
    return false;
}

QVector<StockEntry> StockRegistry::findByMaterialId(const QUuid& materialId) const {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    QVector<StockEntry> result;
    for (const auto& entry : _data) {
        if (entry.materialId == materialId) {
            result.append(entry);
        }
    }
    return result;
}

std::optional<StockEntry> StockRegistry::findByMaterialAndStorage(const QUuid& materialId,
                                                                  const QUuid& storageId) const
{
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    qInfo() << QStringLiteral("🔎 findByMaterialAndStorage keresés indul: materialId=%1, storageId=%2")
                   .arg(materialId.toString(), storageId.toString());

    for (const auto& entry : _data) {
        if (entry.materialId == materialId && entry.storageId == storageId) {
            qInfo() << QStringLiteral("✅ Találat: entryId=%1 aggregálható").arg(entry.entryId.toString());
            return entry;
        }
    }

    qInfo() << QStringLiteral("❌ Nincs találat aggregáláshoz");
    return std::nullopt;
}

void StockRegistry::dumpAll() const {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    qInfo() << "📋 StockRegistry dump:";
    for (const auto& entry : _data) {
        qInfo() << QStringLiteral("  entryId=%1, materialId=%2, storageId=%3, qty=%4")
        .arg(entry.entryId.toString(),
             entry.materialId.toString(),
             entry.storageId.toString())
            .arg(entry.quantity);
    }
}

QVector<StockEntry> StockRegistry::findAllByMaterialAndStorageSorted(const QUuid& materialId,
                                                                     const QUuid& storageId) const {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    QVector<StockEntry> result;
    result.reserve(_data.size());
    for (const auto& e : _data) {
        if (e.materialId == materialId && e.storageId == storageId) result.append(e);
    }
    std::sort(result.begin(), result.end(), [](const StockEntry& a, const StockEntry& b){
        if (a.quantity != b.quantity) return a.quantity < b.quantity;
        return a.entryId.toRfc4122() < b.entryId.toRfc4122();
    });
    return result;
}

std::optional<StockEntry> StockRegistry::findFirstByStorageAndMaterial(const QUuid& storageId,
                                                                       const QUuid& materialId) const
{
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    const StockEntry* best = nullptr;
    for (const auto& e : _data) {
        if (e.storageId != storageId || e.materialId != materialId) continue;
        if (!best || e.quantity < best->quantity) {
            best = &e;
        }
    }
    if (!best) return std::nullopt;
    return *best;
}

bool StockRegistry::isEmpty() const {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    return _data.isEmpty();
}

QVector<StockEntry> StockRegistry::readAll() const {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    return _data;
}

void StockRegistry::setData(const QVector<StockEntry>& v) {
    ScopedPerThreadLock locker(static_cast<void*>(&_mutex), /*recursive=*/true);
    _data = v;
    persist();
}
