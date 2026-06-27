#include "bom_registry.h"

BomRegistry& BomRegistry::instance() {
    static BomRegistry reg;
    return reg;
}
