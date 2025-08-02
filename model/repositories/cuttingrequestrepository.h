#pragma once

#include "model/cuttingplanrequest.h"
#include <QString>
#include <QVector>

class CuttingPlanRequestRegistry;

class CuttingRequestRepository {
public:
    /// Betölti a vágási tervet a beállításokból lekért fájlból
    static bool tryLoadFromSettings(CuttingPlanRequestRegistry& registry);

    /// Betölti a vágási igényeket a megadott fájlból
    static bool loadFromFile(CuttingPlanRequestRegistry& registry, const QString& filePath);

    /// Elmenti a vágási igényeket CSV formátumban
    static bool saveToFile(const CuttingPlanRequestRegistry& registry, const QString& filePath);

    static bool wasLastFileEffectivelyEmpty() {
        return lastFileWasEffectivelyEmpty;
    }
private:
    inline static bool lastFileWasEffectivelyEmpty = false;

    /// Segédmetódus a CSV-ből CuttingPlanRequest-ek létrehozásához
    static QVector<CuttingPlanRequest> loadFromCsv_private(const QString& filepath);

    struct CuttingRequestRow {
        QString barcode;
        int requiredLength;
        int quantity;
        QString ownerName;
        QString externalReference;
    };

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<CuttingPlanRequest> buildCuttingRequestFromRow(const CuttingRequestRow &row, int lineIndex);
    static std::optional<CuttingPlanRequest> convertRowToCuttingRequest(const QVector<QString>& parts, int lineIndex);
};
