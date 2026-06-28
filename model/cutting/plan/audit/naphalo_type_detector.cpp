#include "naphalo_type_detector.h"
#include "product/registry/product_subtype_registry.h"

NaphaloType NaphaloTypeDetector::detect(const QVector<Cutting::Plan::Request>& list)
{
    if (list.isEmpty())
        return NaphaloType::Unknown;

    auto* subtype = ProductSubtypeRegistry::instance().findById(list.first().productSubtypeId);
    if (!subtype)
        return NaphaloType::Unknown;

    QString code = subtype->code.trimmed();

    if (code == "CIP") return NaphaloType::Cipzaros;
    if (code == "SZ")  return NaphaloType::Sines;
    if (code == "BOW") return NaphaloType::Bowdenes;

    return NaphaloType::Unknown;

}
