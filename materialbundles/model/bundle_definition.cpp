#include "bundle_definition.h"
#include "materials/registry/material_registry.h"

std::optional<double> BundleDefinition::computedLength_mm() const
{
    // ha már kiszámoltuk → adjuk vissza
    if (_cachedLength.has_value())
        return _cachedLength;

    if (components.isEmpty())
        return std::nullopt;

    double maxLen = 0.0;

    for (const auto& c : components) {
        const auto* mat = MaterialRegistry::instance().findById(c.materialId);
        if (!mat)
            return std::nullopt;

        double len = mat->rawStockLength_mm();
        maxLen = std::max(maxLen, len);
    }

    _cachedLength = maxLen;   // 🔥 cache-elés
    return _cachedLength;
}



