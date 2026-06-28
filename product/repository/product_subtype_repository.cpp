#include "product_subtype_repository.h"
#include "common/filenamehelper.h"
#include <QUuid>

bool ProductSubtypeRepository::loadFromCSV(ProductSubtypeRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString fn = helper.getProductSubtypeCsvFile();
    CsvReader::FileContext ctx(fn);

    const QVector<ProductSubtype> loaded = loadFromCSV_private(ctx);

    if (ctx.hasErrors())
        zWarning(ctx.toString());

    if (loaded.isEmpty()) {
        zWarning("❌ product_subtype.csv üres vagy hibás");
        return false;
    }

    registry.setData(loaded);
    zInfo(QString("✅ %1 termék-altípus sikeresen importálva a fájlból: %2").arg(loaded.size()).arg(fn));

    return true;
}

std::optional<ProductSubtype>
ProductSubtypeRepository::convertRow(const QVector<QString>& parts,
                                     CsvReader::FileContext& ctx)
{
    if (parts.size() < 2) {
        ctx.addError(ctx.currentLineNumber(), "❌ subtype sor hibás");
        return std::nullopt;
    }

    const QString typeCode = parts[0].trimmed();
    const QString subtypeCode = parts[1].trimmed();
    const QString subtypeName = parts[2].trimmed();


    const auto* type = ProductTypeRegistry::instance().findByCode(typeCode);
    if (!type) {
        ctx.addError(ctx.currentLineNumber(),
                     "❌ Ismeretlen product type: " + typeCode);
        return std::nullopt;
    }

    ProductSubtype s;
    s.id = QUuid::createUuid();
    s.typeId = type->id;
    s.code = subtypeCode;
    s.name = subtypeName;
    return s;
}

QVector<ProductSubtype>
ProductSubtypeRepository::loadFromCSV_private(CsvReader::FileContext& ctx)
{
    return CsvReader::readAndConvert<ProductSubtype>(ctx, convertRow, true);
}
