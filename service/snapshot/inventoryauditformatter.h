#pragma once

#include "inventoryauditmodel.h"

class InventoryAuditFormatter
{
public:
    static QString toText(
        const InventoryAuditModel& model);
};