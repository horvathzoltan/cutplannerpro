#include "bom_repository.h"
#include "common/filenamehelper.h"
#include <QUuid>

bool BomRepository::loadFromCSV(BomRegistry& registry) {
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) return false;

    const QString fn = helper.getBomCsvFile();   // ezt mindjárt hozzáadjuk
    CsvReader::FileContext ctx(fn);

    const QVector<BomEntry> loaded = loadFromCSV_private(ctx);

    if (ctx.hasErrors())
        zWarning(ctx.toString());

    if (loaded.isEmpty()) {
        zWarning("❌ bom.csv üres vagy hibás");
        return false;
    }

    registry.setData(loaded);
    zInfo(QString("✅ %1 BOM-lista sikeresen importálva a fájlból: %2").arg(loaded.size()).arg(fn));

    return true;
}

std::optional<BomEntry>
BomRepository::convertRow(const QVector<QString>& parts,
                          CsvReader::FileContext& ctx)
{
    if (parts.size() < 4) {
        ctx.addError(ctx.currentLineNumber(),
                     QString("❌ BOM sor hibás (%1 mező, legalább 4 kell)").arg(parts.size()));
        return std::nullopt;
    }

    const QString typeCode = parts[0].trimmed();
    const QString subtypeCode = parts[1].trimmed();

    const QString familyStr = parts[2].trimmed();
    const QString qtyStr = parts[3].trimmed();

    const auto* type = ProductTypeRegistry::instance().findByCode(typeCode);
    if (!type) {
        ctx.addError(ctx.currentLineNumber(), "❌ Ismeretlen product type: " + typeCode);
        return std::nullopt;
    }

    const auto* subtype = ProductSubtypeRegistry::instance().findByCode(subtypeCode);
    if (!subtype) {
        ctx.addError(ctx.currentLineNumber(), "❌ Ismeretlen product subtype: " + subtypeCode);
        return std::nullopt;
    }

    MaterialFamily fam = MaterialFamilyUtils::fromString(familyStr);
    if (fam == MaterialFamily::Unknown) {
        ctx.addError(ctx.currentLineNumber(), "❌ Ismeretlen family: " + familyStr);
        return std::nullopt;
    }

    bool ok = false;
    double qty = qtyStr.toDouble(&ok);
    if (!ok || qty <= 0) {
        ctx.addError(ctx.currentLineNumber(), "❌ Hibás quantity: " + qtyStr);
        return std::nullopt;
    }

    BomEntry e;
    e.id = QUuid::createUuid();
    e.productTypeId = type->id;
    e.productSubtypeId = subtype->id;
    e.family = fam;
    e.quantity = qty;

    return e;
}

QVector<BomEntry>
BomRepository::loadFromCSV_private(CsvReader::FileContext& ctx)
{
    return CsvReader::readAndConvert<BomEntry>(ctx, convertRow, true);
}
