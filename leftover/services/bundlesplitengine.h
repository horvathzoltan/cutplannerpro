#pragma once

#include "leftover/model/leftoverstockentry.h"
#include "materialbundles/model/bundle_definition.h"
#include "materialbundles/registry/bundle_registry.h"

struct BundleSplitResult {
    LeftoverStockEntry updatedOriginal;                 // hiányos bundle
    QVector<LeftoverStockEntry> newLeftovers;           // kivett komponensekből képzett leftoverek
};

class BundleSplitEngine
{
public:
    static BundleSplitResult applySplit(
        const LeftoverStockEntry& original,
        const QVector<BundleComponentLength>& remaining,
        const QVector<BundleComponentLength>& removed);
};
