#include "reusablestockentry.h"
#include "registries/materialregistry.h"
#include "common/grouputils.h"

const MaterialMaster* ReusableStockEntry::master() const {
    if (materialId.isNull())
        return nullptr;

    return MaterialRegistry::instance().findById(materialId);
}

QString ReusableStockEntry::name() const {
    const auto* m = master();
    return m ? m->name : "(?)";
}

QString ReusableStockEntry::materialBarcode() const {
    const auto* m = master();
    return m ? m->barcode : "(?)";
}

QString ReusableStockEntry::reusableBarcode() const {
    return barcode.isEmpty()? "(?)" : barcode;
}

MaterialType ReusableStockEntry::type() const {
    const auto* m = master();
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}

QString ReusableStockEntry::groupName() const {
    return GroupUtils::groupName(materialId);
}

QColor ReusableStockEntry::groupColor() const {
    return GroupUtils::colorForGroup(materialId);
}

bool ReusableStockEntry::operator==(const ReusableStockEntry& other) const {
    return materialId == other.materialId &&
           availableLength_mm == other.availableLength_mm;
}

QString ReusableStockEntry::sourceAsString() const
{
    if (source == LeftoverSource::Manual)
        return "Manuális";
    if (source == LeftoverSource::Optimization)
        return optimizationId.has_value()
                   ? QString("Op:%1").arg(*optimizationId)
                   : "Op:?";
    return "Ismeretlen";
}

