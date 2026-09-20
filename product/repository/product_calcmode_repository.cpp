#include "product/repository/product_calcmode_repository.h"
#include "common/filehelper.h"
#include "common/filenamehelper.h"
#include "common/logger.h"

#include <QFile>
#include <QTextStream>

// --- Stage 1: Convert parent row ---

std::optional<ProductCalcModeRepository::ParentRow>
ProductCalcModeRepository::convertParentRow(
    const QVector<QString>& parts,
    CsvReader::FileContext& ctx)
{
    if (parts.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: parent sor üres.");
        return std::nullopt;
    }

    QStringList ts = parts[0].split(";", Qt::SkipEmptyParts);
    if (ts.size() < 2) {
        ctx.addError(ctx.currentLineNumber(),
                     "❌ MSFF: parent sor hibás (type;subtype szükséges).");
        return std::nullopt;
    }

    ParentRow row {
        .typeCode = ts[0].trimmed(),
        .subtypeCode = ts[1].trimmed()
    };

    return row;
}

// --- Stage 1: Convert child row ---

std::optional<ProductCalcModeRepository::ChildRow>
ProductCalcModeRepository::convertChildRow(
    const QVector<QString>& parts,
    CsvReader::FileContext& ctx)
{
    if (parts.isEmpty()) {
        ctx.addError(ctx.currentLineNumber(), "❌ MSFF: child sor üres.");
        return std::nullopt;
    }

    ChildRow row { .modeString = parts[0].trimmed() };
    return row;
}


// --- Stage 2: Build ProductCalcModes objektum ---

std::optional<ProductCalcModes>
ProductCalcModeRepository::buildEntry(
    const ParentRow& parent,
    const QList<ChildRow>& children,
    CsvReader::FileContext& ctx)
{
    ProductCalcModes entry;
    entry.typeCode = parent.typeCode;
    entry.subtypeCode = parent.subtypeCode;

    bool defaultSet = false;

    for (const auto& ch : children) {
        QString raw = ch.modeString;

        bool isDefault = false;

        // --- ÚJ: "Gyartasi: default" formátum felismerése ---
        if (raw.contains(":")) {
            auto parts = raw.split(":", Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                QString flag = parts[1].trimmed().toLower();
                if (flag == "default")
                    isDefault = true;

                raw = parts[0].trimmed();
            }
        }

        // --- mód parse ---
        auto maybeMode = SizeCalcModeUtils::parseSizeCalcMode(raw);

        entry.modes.append(maybeMode);

        if (isDefault) {
            entry.defaultMode = maybeMode;
            defaultSet = true;
        }
    }

    // --- VALIDÁCIÓ: ha több mód van → kötelező a default ---
    if (entry.modes.size() > 1 && !defaultSet) {
        ctx.addError(ctx.currentLineNumber(),
                     QString("❌ MSFF: több calcMode van, de nincs default megadva (%1;%2).")
                         .arg(entry.typeCode)
                         .arg(entry.subtypeCode));
        return std::nullopt;
    }

    // --- ha csak egy mód van → automatikusan default ---
    if (entry.modes.size() == 1) {
        entry.defaultMode = entry.modes.first();
    }

    // --- ha nincs mód egyáltalán → Unknown
    if (entry.modes.isEmpty()) {
        entry.modes.append(SizeCalcMode::Unknown);
        entry.defaultMode = SizeCalcMode::Unknown;
    }

    return entry;
}


// --- Stage 3: MSFF beolvasás + szekciók szétválasztása ---

QList<QList<QVector<QString>>>
ProductCalcModeRepository::readMsffSections(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        zWarning("❌ Nem sikerült megnyitni az MSFF fájlt: " + filepath);
        return {};
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    auto sepResult = FileHelper::detectSeparatorMsff(&in);
    if (sepResult.hasError || sepResult.separator.isNull()) {
        zWarning("❌ MSFF: szeparátor detektálás sikertelen.");
        return {};
    }

    file.seek(0);
    in.seek(0);

    auto allRows = FileHelper::parseCSV(&in, sepResult.separator, true);
    return FileHelper::splitSections(allRows, sepResult.headerLineCount);
}

// --- Stage 3: Parse sections + registry feltöltése ---

bool ProductCalcModeRepository::parseMsffSections(
    const QList<QList<QVector<QString>>>& sections,
    ProductCalcModeRegistry& registry)
{
    for (const auto& sec : sections) {
        if (sec.isEmpty()) continue;

        CsvReader::FileContext ctx("product_calcmodes");

        // --- Parent ---
        auto maybeParent = convertParentRow(sec[0], ctx);
        if (!maybeParent.has_value()) {
            zWarning("❌ MSFF: hibás parent sor.");
            return false;
        }

        ParentRow parent = maybeParent.value();

        // --- Children ---
        QList<ChildRow> children;
        for (int i = 1; i < sec.size(); ++i) {
            auto maybeChild = convertChildRow(sec[i], ctx);
            if (maybeChild.has_value())
                children.append(maybeChild.value());
        }

        auto maybeEntry = buildEntry(parent, children, ctx);
        if (!maybeEntry.has_value()) {
            zWarning("❌ MSFF: hibás calcMode entry.");
            return false;
        }

        registry.registerEntry(maybeEntry.value());
    }

    return true;
}

// --- Entry point ---

bool ProductCalcModeRepository::loadFromMsff(ProductCalcModeRegistry& registry)
{
    const auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString path = helper.getProductCalculationModesMsffFile(); // product_calcmodes.msff

    auto sections = readMsffSections(path);
    if (sections.isEmpty()) {
        zWarning("❌ MSFF: üres vagy hibás fájl.");
        return false;
    }

    return parseMsffSections(sections, registry);
}
