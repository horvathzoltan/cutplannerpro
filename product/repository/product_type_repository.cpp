#include "product_type_repository.h"
#include "common/filenamehelper.h"
#include <QUuid>

bool ProductTypeRepository::loadFromCSV(ProductTypeRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString fn = helper.getProductTypeCsvFile();
    CsvReader::FileContext ctx(fn);

    const QVector<ProductType> loaded = loadFromCSV_private(ctx);

    if (ctx.hasErrors())
        zWarning(ctx.toString());

    if (loaded.isEmpty()) {
        zWarning("❌ product_type.csv üres vagy hibás");
        return false;
    }

    registry.setData(loaded);
    zInfo(QString("✅ %1 terméktípus sikeresen importálva a fájlból: %2").arg(loaded.size()).arg(fn));

    return true;
}

std::optional<ProductType>
ProductTypeRepository::convertRow(const QVector<QString>& parts,
                                  CsvReader::FileContext& ctx)
{
    if (parts.size() < 1) {
        ctx.addError(ctx.currentLineNumber(), "❌ Hiányzó product type név");
        return std::nullopt;
    }

    ProductType t;
    t.id = QUuid::createUuid();
    t.name = parts[0].trimmed();
    t.code = parts[1].trimmed();
    return t;
}

QVector<ProductType>
ProductTypeRepository::loadFromCSV_private(CsvReader::FileContext& ctx)
{
    return CsvReader::readAndConvert<ProductType>(ctx, convertRow, true);
}
