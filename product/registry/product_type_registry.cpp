#include "product_type_registry.h"


ProductTypeRegistry& ProductTypeRegistry::instance() {
    static ProductTypeRegistry reg;
    return reg;
}
