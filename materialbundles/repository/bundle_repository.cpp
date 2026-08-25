#include "materialbundles/repository/bundle_repository.h"
#include "common/filenamehelper.h"
#include <QMap>
#include <QDebug>

// --- Stage 1: Convert ---

std::optional<BundleRepository::BundleRow>
BundleRepository::convertRowToBundleRow(const QVector<QString>& parts,
                                        CsvReader::FileContext& ctx)
{
    if (parts.size() < 2) {
        ctx.addError(ctx.currentLineNumber(), "❌ Invalid bundle row");
        return std::nullopt;
    }

    BundleRow row {
        .bundleCode = parts[0].trimmed(),
        .bundleName = parts[1].trimmed()
    };

    return row;
}

std::optional<BundleRepository::BundleComponentRow>
BundleRepository::convertRowToBundleComponentRow(const QVector<QString>& parts,
                                                 CsvReader::FileContext& ctx)
{
    // bundleCode;materialBarcode;count
    if (parts.size() < 3) {
        ctx.addError(ctx.currentLineNumber(), "❌ Invalid bundle component row (expected 3 fields)");
        return std::nullopt;
    }

    BundleComponentRow row {
        .bundleCode      = parts[0].trimmed(),
        .materialBarcode = parts[1].trimmed(),
        .count           = parts[2].trimmed().toInt()
        // nincs baseLength_mm definíció szinten
    };

    return row;
}


// --- Stage 2: Build ---

std::optional<BundleDefinition>
BundleRepository::buildBundleFromRow(const BundleRow& row,
                                     CsvReader::FileContext& ctx)
{
    if (row.bundleCode.isEmpty() || row.bundleName.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(), "❌ Missing bundle fields");
        return std::nullopt;
    }

    BundleDefinition def;
    def.id   = QUuid::createUuid();
    def.code = row.bundleCode;
    def.name = row.bundleName;

    return def;
}

std::optional<BundleComponent>
BundleRepository::buildComponentFromRow(const BundleComponentRow& row,
                                        CsvReader::FileContext& ctx)
{
    const auto* mat = MaterialRegistry::instance().findByBarcode(row.materialBarcode);
    if (!mat) {
        qWarning() << "⚠️ Unknown material barcode in bundle:" << row.materialBarcode;
        return std::nullopt;
    }

    BundleComponent comp;
    comp.materialId = mat->id;
    comp.count      = row.count;

    // ⚠️ Definíció szinten NEM töltünk baseLength-et.
    // A hossz a MaterialMaster.stockLength_mm-ből jön,
    // illetve vágás/leftover alatt példány szinten kerül tárolásra.

    return comp;
}



// --- Stage 3: Load & Assemble ---

QVector<BundleRepository::BundleRow>
BundleRepository::loadBundleRows(CsvReader::FileContext& ctx)
{
    return CsvReader::readAndConvert<BundleRow>(ctx, convertRowToBundleRow);
}

QVector<BundleRepository::BundleComponentRow>
BundleRepository::loadComponentRows(CsvReader::FileContext& ctx)
{
    return CsvReader::readAndConvert<BundleComponentRow>(ctx, convertRowToBundleComponentRow);
}

bool BundleRepository::loadFromCsv(BundleRegistry& registry)
{
    const auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString metaPath     = helper.getBundleCsvFile();           // bundles.csv
    const QString compPath     = helper.getBundleComponentsCsvFile(); // bundle_components.csv

    CsvReader::FileContext metaCtx(metaPath);
    CsvReader::FileContext compCtx(compPath);

    const auto bundleRows    = loadBundleRows(metaCtx);
    const auto componentRows = loadComponentRows(compCtx);

    QMap<QString, BundleDefinition> bundleMap;

    // --- Build bundle definitions ---
    for (int i = 0; i < bundleRows.size(); ++i) {
        metaCtx.setCurrentLineNumber(i + 1);

        auto opt = buildBundleFromRow(bundleRows[i], metaCtx);
        if (!opt.has_value()) continue;

        const auto& row = bundleRows[i];

        if (bundleMap.contains(row.bundleCode)) {
            qWarning() << "⚠️ Duplicate bundle:" << row.bundleCode;
        }

        bundleMap.insert(row.bundleCode, opt.value());
    }

    // --- Build components ---
    for (int i = 0; i < componentRows.size(); ++i) {
        compCtx.setCurrentLineNumber(i + 1);

        const auto& row = componentRows[i];

        if (!bundleMap.contains(row.bundleCode)) {
            qWarning() << "⚠️ Component for undefined bundle:" << row.bundleCode;
            continue;
        }

        auto compOpt = buildComponentFromRow(row, compCtx);
        if (!compOpt.has_value()) continue;

        bundleMap[row.bundleCode].components.append(compOpt.value());
    }

    // --- Register bundles ---
    for (auto it = bundleMap.constBegin(); it != bundleMap.constEnd(); ++it) {
        registry.registerBundle(it.value());
    }

    return true;
}
