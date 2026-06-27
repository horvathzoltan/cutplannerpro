#include "product/registry/product_subtype_registry.h"

ProductSubtypeRegistry& ProductSubtypeRegistry::instance() {
    static ProductSubtypeRegistry reg;
    return reg;
}
