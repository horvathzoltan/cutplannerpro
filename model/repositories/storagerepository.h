#pragma once

#include <QString>
#include <QVector>
#include "../storageentry.h"
#include "../registries/storageregistry.h"

class StorageRepository {
public:
    static bool loadFromCSV(StorageRegistry& registry);

private:
    struct StorageRow {
        QString uuidStr;
        QString name;
        QString typeStr;
        QString parentIdStr;
        QString barcode;
        QString comment;
    };


    static QVector<StorageEntry> loadFromCSV_private(const QString& filePath);
    static std::optional<StorageEntry> convertRowToStorage(const QVector<QString>& parts, int lineIndex);
    static std::optional<StorageRow> convertRowToStorageRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<StorageEntry> buildStorageFromRow(const StorageRow& row, int lineIndex);
};
