#include "stockentry.h"
#include "registries/materialregistry.h"

#include "common/grouputils.h" // biztosan benne van

const MaterialMaster* StockEntry::master() const {
    if (materialId.isNull())
        return nullptr;

    const MaterialMaster *opt = MaterialRegistry::instance().findById(materialId);
    return opt;
}

QString StockEntry::materialName() const {
    const auto* m = master();
    return m ? m->name : "(?)";
}

QString StockEntry::mterialBarcode() const {
    const auto* m = master();
    return m ? m->barcode : "(?)";
}

MaterialType StockEntry::materialType() const {
    const auto* m = master();
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}

QString StockEntry::materialGroupName() const {
    return GroupUtils::groupName(materialId);
}

QColor StockEntry::materialGroupColor() const {
    return GroupUtils::colorForGroup(materialId);
}
