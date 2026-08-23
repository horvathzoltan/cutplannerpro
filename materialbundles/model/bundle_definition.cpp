#include "bundle_definition.h"
#include "materials/registry/material_registry.h"

std::optional<double> BundleDefinition::computedLength_mm() const
{
    if (components.isEmpty())
        return std::nullopt;

    // 1) lekérjük az első komponens anyag hosszát
    const auto* firstMat = MaterialRegistry::instance().findById(components[0].materialId);
    if (!firstMat)
        return std::nullopt;

    double firstLen = firstMat->stockLength_mm;

    // 2) ellenőrizzük, hogy minden komponens hossza azonos-e
    for (const auto& c : components) {
        const auto* mat = MaterialRegistry::instance().findById(c.materialId);
        if (!mat)
            return std::nullopt;

        if (mat->stockLength_mm != firstLen)
            return std::nullopt;   // nincs értelmezhető bundleLength
    }

    // 3) minden komponens hossza azonos → van bundleLength
    return firstLen;
}
