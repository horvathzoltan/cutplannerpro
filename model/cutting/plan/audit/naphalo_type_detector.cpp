#include "naphalo_type_detector.h"
#include "naphalo_material_aggregator.h"

NaphaloType NaphaloTypeDetector::detect(const QVector<Cutting::Plan::Request>& list)
{
    if (list.isEmpty())
        return NaphaloType::Unknown;

    auto materials = NaphaloMaterialAggregator::aggregate(list);

    int labCount = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::Lab) * 2;
    int czCount  = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::CipzarosZaro);
    int szCount  = NaphaloMaterialAggregator::sumGroup(materials, NaphaloGroup::SinesZaro);

    // CIPZÁROS
    if (czCount > 0)
        return NaphaloType::Cipzaros;

    // SÍNES
    if (szCount > 0 && labCount > 0)
        return NaphaloType::Sines;

    // BOWDENES
    if (szCount > 0 && labCount == 0)
        return NaphaloType::Bowdenes;

    return NaphaloType::Unknown;
}
