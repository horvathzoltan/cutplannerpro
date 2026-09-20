#include "product/registry/product_calcmode_registry.h"
#include "common/logger.h"

// --- Private helpers ---

QString ProductCalcModeRegistry::makeKey(const QString& type,
                                         const QString& subtype) const
{
    return type.trimmed() + ";" + subtype.trimmed();
}

// --- Singleton ---

ProductCalcModeRegistry& ProductCalcModeRegistry::instance()
{
    static ProductCalcModeRegistry inst;
    return inst;
}

// --- Public API ---

void ProductCalcModeRegistry::clearAll()
{
    _data.clear();
}

void ProductCalcModeRegistry::registerEntry(const ProductCalcModes& entry)
{
    QString key = makeKey(entry.typeCode, entry.subtypeCode);
    _data[key] = entry;
}

QVector<SizeCalcMode> ProductCalcModeRegistry::getModes(const QString& typeCode,
                                                        const QString& subtypeCode) const
{
    QString key = makeKey(typeCode, subtypeCode);
    if (_data.contains(key))
        return _data[key].modes;

    return { SizeCalcMode::Unknown };   // fallback
}

SizeCalcMode ProductCalcModeRegistry::getDefault(const QString& typeCode,
                                                 const QString& subtypeCode) const
{
    QString key = makeKey(typeCode, subtypeCode);
    if (_data.contains(key))
        return _data[key].defaultMode;

    return SizeCalcMode::Unknown;       // fallback
}

QList<ProductCalcModes> ProductCalcModeRegistry::readAll() const
{
    return _data.values();
}


void ProductCalcModeRegistry::debugDump() const
{
    zInfo("==============================================");
    zInfo("🔍 ProductCalcModeRegistry dump indul");
    zInfo("==============================================");

    if (_data.isEmpty()) {
        zWarning("⚠️ Nincsenek calcMode bejegyzések a registry-ben.");
        return;
    }

    for (const auto& entry : _data) {

        zInfo(QString("📦 Típus/Altípus: %1 / %2")
                  .arg(entry.typeCode)
                  .arg(entry.subtypeCode));

        // módok
        if (entry.modes.isEmpty()) {
            zWarning("   ⚠️ Nincsenek calcMode-ok.");
        } else {
            zInfo("   Módok:");
            for (auto m : entry.modes) {
                zInfo(QString("      ➤ %1")
                          .arg(SizeCalcModeUtils::toString(m)));
            }
        }

        // default
        zInfo(QString("   Default: %1")
                  .arg(SizeCalcModeUtils::toString(entry.defaultMode)));

        zInfo(""); // üres sor
    }

    zInfo("==============================================");
    zInfo("🔚 ProductCalcModeRegistry dump vége");
    zInfo("==============================================");
}