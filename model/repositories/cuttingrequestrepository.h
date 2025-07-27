#pragma once

#include "model/cuttingrequest.h"
#include <QString>
#include <QVector>

class CuttingRequestRegistry;

class CuttingRequestRepository {
public:
    /// Betölti a vágási tervet a beállításokból lekért fájlból
    static bool tryLoadFromSettings(CuttingRequestRegistry& registry);

    /// Betölti a vágási igényeket a megadott fájlból
    static bool loadFromFile(CuttingRequestRegistry& registry, const QString& filePath);

    /// Elmenti a vágási igényeket CSV formátumban
    static bool saveToFile(const CuttingRequestRegistry& registry, const QString& filePath);

private:
    /// Segédmetódus a CSV-ből CuttingRequest-ek létrehozásához
    static QVector<CuttingRequest> loadFromCsv(const QString& filepath);

    struct CuttingRequestRow {
        QString barcode;
        int requiredLength;
        int quantity;
        QString ownerName;
        QString externalReference;
    };

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<CuttingRequest> buildCuttingRequestFromRow(const CuttingRequestRow &row, int lineIndex);
    static std::optional<CuttingRequest> convertRowToCuttingRequest(const QVector<QString>& parts, int lineIndex);
};
