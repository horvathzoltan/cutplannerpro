#include "powder_consumption_repository.h"
#include <QFile>
#include <QTextStream>
#include <common/filenamehelper.h>

bool PowderConsumptionRepository::loadFromCSV(PowderConsumptionRegistry& registry)
{
    auto& helper = FileNameHelper::instance();
    if (!helper.isInited()) {
        zWarning("❌ FileNameHelper nincs inicializálva.");
        return false;
    }

    const QString fn = helper.getPowderConsumptionCsvFile();
    if (fn.isEmpty()) {
        zWarning("❌ Nem található a powder_consumption.csv fájl.");
        return false;
    }

    CsvReader::FileContext ctx(fn);
    const QVector<PowderConsumptionModel> loaded = loadFromCSV_private(ctx);

    if (ctx.hasErrors()) {
        zWarning(QString("⚠️ Hibák az importálás során (%1 sor):").arg(ctx.errorsSize()));
        zWarning(ctx.toString());
    }

    if (loaded.isEmpty()) {
        zWarning("❌ A powder_consumption.csv üres vagy hibás.");
        return false;
    }

    registry.setData(loaded);
    zInfo(QString("✅ %1 festési norma betöltve: %2").arg(loaded.size()).arg(fn));

    return true;
}

std::optional<PowderConsumptionRepository::Row>
PowderConsumptionRepository::parseRow(const QVector<QString>& parts,
                                      CsvReader::FileContext& ctx)
{
    if (parts.size() < 6) {
        ctx.addError(ctx.currentLineNumber(),
                     QString("❌ Kevés mező (%1), legalább 6 kell").arg(parts.size()));
        return std::nullopt;
    }

    Row r;
    r.typeCode       = parts[0].trimmed();
    r.subtypeCode    = parts[1].trimmed();
    r.lengthStr      = parts[2].trimmed();
    r.weightStr      = parts[3].trimmed();
    r.colorMultStr   = parts[4].trimmed();
    r.surfaceMultStr = parts[5].trimmed();

    return r;
}

std::optional<PowderConsumptionModel>
PowderConsumptionRepository::buildModel(const Row& row,
                                        CsvReader::FileContext& ctx)
{

    const ProductType* type = ProductTypeRegistry::instance().findByCode(row.typeCode);
    const ProductSubtype* subtype = ProductSubtypeRegistry::instance().findByCode(row.subtypeCode);

    if (!type || !subtype) {
        ctx.addError(ctx.currentLineNumber(),
                     QString("❌ Ismeretlen type/subtype: %1/%2")
                         .arg(row.typeCode)
                         .arg(row.subtypeCode));
        return std::nullopt;
    }

    auto typeId = type->id;
    auto subtypeId = subtype->id;

    if (typeId.isNull() || subtypeId.isNull()) {
        ctx.addError(ctx.currentLineNumber(),
                     QString("❌ Ismeretlen type/subtype: %1/%2")
                         .arg(row.typeCode)
                         .arg(row.subtypeCode));
        return std::nullopt;
    }

    bool okLen=false, okW=false, okC=false, okS=false;

    double len = row.lengthStr.toDouble(&okLen);
    double w   = row.weightStr.toDouble(&okW);
    double cm  = row.colorMultStr.toDouble(&okC);
    double sm  = row.surfaceMultStr.toDouble(&okS);

    if (!okLen || !okW) {
        ctx.addError(ctx.currentLineNumber(),
                     "❌ length/weight nem szám");
        return std::nullopt;
    }

    PowderConsumptionModel m;
    m.productTypeId    = typeId;
    m.productSubtypeId = subtypeId;
    m.length           = len;
    m.weight           = w;
    m.colorMultiplier  = okC ? cm : 1.0;
    m.surfaceMultiplier= okS ? sm : 1.0;

    return m;
}

QVector<PowderConsumptionModel>
PowderConsumptionRepository::loadFromCSV_private(CsvReader::FileContext& ctx)
{
    return CsvReader::readAndConvert<PowderConsumptionModel>(
        ctx,
        [](const QVector<QString>& parts, CsvReader::FileContext& ctx)
        {
            auto rowOpt = parseRow(parts, ctx);
            if (!rowOpt.has_value())
                return std::optional<PowderConsumptionModel>{};

            return buildModel(rowOpt.value(), ctx);
        },
        true
        );
}
