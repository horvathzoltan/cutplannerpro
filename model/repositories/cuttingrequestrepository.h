#pragma once

#include "model/cutting/plan/request.h"
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

    /// Segédmetódus a CSV-ből Request-ek létrehozásához
    static QVector<Cutting::Plan::Request> loadFromCsv_private(const QString& filepath);

    struct CuttingRequestRow {
        QString barcode;
        int requiredLength;
        int quantity;
        QString ownerName;
        QString externalReference;
    };

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow(const QVector<QString>& parts, int lineIndex);
    static std::optional<Cutting::Plan::Request> buildCuttingRequestFromRow(const CuttingRequestRow &row, int lineIndex);
    static std::optional<Cutting::Plan::Request> convertRowToCuttingRequest(const QVector<QString>& parts, int lineIndex);
};
