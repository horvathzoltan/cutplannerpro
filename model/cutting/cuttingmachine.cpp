#include "cuttingmachine.h"

// CuttingMachine::CuttingMachine() {}

void CuttingMachine::addMaterialType(const MaterialType &v)
{
    compatibleMaterials.append(v);
}
