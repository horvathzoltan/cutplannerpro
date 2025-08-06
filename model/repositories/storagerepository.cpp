#include "storagerepository.h"
#include "../storagetype.h"
#include "../registries/storageregistry.h"
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <common/filehelper.h>
#include <common/filenamehelper.h>
#include <QDebug>

bool StorageRepository::loadFromCSV(StorageRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString fn = helper.getStorageCsvFile();
    if (fn.isEmpty()) {
        qWarning("⚠️ A storage CSV fájl nem található.");
        return false;
    }

    const QVector<StorageEntry> loaded = loadFromCSV_private(fn);
    if (loaded.isEmpty()) {
        qWarning("⚠️ A storage.csv fájl üres vagy hibás.");
        return false;
    }

    registry.setData(loaded);
    return true;
}

QVector<StorageEntry> StorageRepository::loadFromCSV_private(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "❌ Nem sikerült megnyitni a CSV fájlt:" << filePath;
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    const QList<QVector<QString>> rows = FileHelper::parseCSV(&in, ';');

    QVector<StorageEntry> result;
    for (int i = 1; i < rows.size(); ++i) { // skip header
        const auto maybeStorage = convertRowToStorage(rows[i], i + 1);
        if (maybeStorage.has_value())
            result.append(maybeStorage.value());
    }

    return result;
}

std::optional<StorageEntry> StorageRepository::convertRowToStorage(const QVector<QString>& parts, int lineIndex) {
    const auto rowOpt = convertRowToStorageRow(parts, lineIndex);
    if (!rowOpt.has_value()) return std::nullopt;

    return buildStorageFromRow(rowOpt.value(), lineIndex);
}

std::optional<StorageRepository::StorageRow>
StorageRepository::convertRowToStorageRow(const QVector<QString>& parts, int lineIndex) {
    if (parts.size() < 6) {
        qWarning() << QString("⚠️ Sor %1: hiányzó mezők (legalább 6)").arg(lineIndex);
        return std::nullopt;
    }

    StorageRow row;
    row.uuidStr     = parts[0].trimmed();
    row.name        = parts[1].trimmed();
    row.typeStr     = parts[2].trimmed();
    row.parentIdStr = parts[3].trimmed();
    row.barcode     = parts[4].trimmed();
    row.comment     = parts[5].trimmed();

    if (row.uuidStr.isEmpty() || row.name.isEmpty() || row.typeStr.isEmpty()) {
        qWarning() << QString("⚠️ Sor %1: kötelező mező hiányzik").arg(lineIndex);
        return std::nullopt;
    }

    return row;
}


std::optional<StorageEntry>
StorageRepository::buildStorageFromRow(const StorageRow& row, int lineIndex) {
    QUuid id, parentId;

    if (!QUuid::fromString(row.uuidStr).isNull())
        id = QUuid::fromString(row.uuidStr);
    else {
        qWarning() << QString("⚠️ Sor %1: érvénytelen UUID").arg(lineIndex);
        return std::nullopt;
    }

    if (!row.parentIdStr.isEmpty()) {
        parentId = QUuid::fromString(row.parentIdStr);
        if (parentId.isNull()) {
            qWarning() << QString("⚠️ Sor %1: érvénytelen parent_id").arg(lineIndex);
            return std::nullopt;
        }
    }

    StorageEntry entry;
    entry.id        = id;
    entry.name      = row.name;
    entry.type      = StorageType::fromString(row.typeStr);
    entry.parentId  = parentId;
    entry.barcode   = row.barcode;
    entry.comment   = row.comment;

    return entry;
}

