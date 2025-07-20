#include "stockentry.h"
#include "materialregistry.h"

#include "common/grouputils.h" // biztosan benne van

const MaterialMaster* StockEntry::master() const {
    if (materialId.isNull())
        return nullptr;

    const MaterialMaster *opt = MaterialRegistry::instance().findById(materialId);
    return opt;
}

QString StockEntry::name() const {
    const auto* m = master();
    return m ? m->name : "(?)";
}

QString StockEntry::barcode() const {
    const auto* m = master();
    return m ? m->barcode : "(?)";
}

MaterialType StockEntry::type() const {
    const auto* m = master();
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}

QString StockEntry::groupName() const {
    return GroupUtils::groupName(materialId);
}

QColor StockEntry::groupColor() const {
    return GroupUtils::colorForGroup(materialId);
}
