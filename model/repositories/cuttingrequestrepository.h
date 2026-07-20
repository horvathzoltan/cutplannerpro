#pragma once

#include "../../common/csvimporter.h"
#include "../cutting/plan/request.h"
#include <QString>
#include <QVector>

class CuttingPlanRequestRegistry;

enum class CuttingPlanLoadResult {
    Success,
    NoFileConfigured,
    FileMissing,
    LoadError
};

class CuttingRequestRepository {
public:
    /// Betölti a vágási tervet a beállításokból lekért fájlból
    static CuttingPlanLoadResult tryLoadFromSettings(CuttingPlanRequestRegistry& registry);

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
    static QVector<Cutting::Plan::Request> loadFromCsv_private(CsvReader::FileContext& ctx);

    struct CuttingRequestRow {
        QString externalReference;   ///< Külső hivatkozás / tételszám
        QString ownerName;           ///< Megrendelő neve
        int fullWidth_mm = 0;        ///< Teljes szélesség mm-ben
        int fullHeight_mm = 0;       ///< Teljes magasság mm-ben
        int requiredLength = 0;      ///< Nominális vágási hossz
        QString toleranceStr;        ///< Tűrés szöveges formátumban (CSV-ből)
        int quantity = 0;            ///< Szükséges darabszám
        QString handlerSide; ///< Kezelő oldal (Left/Right)
        QString requiredColorName;   ///< Szín igény (opcionális)
        QString barcode;             ///< Anyag azonosító (lookuphoz)
        QString relevantDimStr;      ///< Releváns dimenzió (Width/Height)
        bool isMeasurementNeeded = false; ///< Mérési terv jelző
        int leftCount = 0;           ///< J darabszám (V2)
        int rightCount = 0;          ///< B darabszám (V2)
        //QString subtypeStr;         ///< Altípus szöveges formátumban (V2)
        QString typeCode;            ///< Terméktípus kód (ProductType.code)
        QString subtypeCode;         ///< Termék altípus kód (ProductSubtype.code)
        QDate dueDate;
        QString attributesStr;
    };

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow_V1(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<Cutting::Plan::Request> convertRowToCuttingRequest_V1(const QVector<QString>& parts,  CsvReader::FileContext& ctx);

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow_V2(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<Cutting::Plan::Request> convertRowToCuttingRequest_V2(const QVector<QString>& parts,  CsvReader::FileContext& ctx);

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow_V3(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<Cutting::Plan::Request> convertRowToCuttingRequest_V3(const QVector<QString>& parts,  CsvReader::FileContext& ctx);

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow_V4(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<Cutting::Plan::Request> convertRowToCuttingRequest_V4(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    static std::optional<CuttingRequestRow> convertRowToCuttingRequestRow_V5(const QVector<QString>& parts, CsvReader::FileContext& ctx);
    static std::optional<Cutting::Plan::Request> convertRowToCuttingRequest_V5(const QVector<QString>& parts, CsvReader::FileContext& ctx);

    static std::optional<Cutting::Plan::Request> buildCuttingRequestFromRow(const CuttingRequestRow &row, CsvReader::FileContext& ctx);

    enum class CSVVersion {
        V1_OldHandlerSide,
        V2_LeftRightSubtype,
        V3_WithDueDate,
        V4_ProductVariant,
        V5_Attributes,
        Unknown
    };

    static CuttingRequestRepository::CSVVersion detectCsvVersion(const QString& filepath);
    static QString serializeAttributes(const QMap<QString, QString>& attrs);
    static QMap<QString, QString> parseAttributes(const QString& s);
};
