#include "stockentry.h"
#include "materialregistry.h"

const MaterialMaster* StockEntry::master() const {
    auto opt = MaterialRegistry::instance().findById(materialId);
    return opt ? &(*opt) : nullptr;
}

QString StockEntry::name() const {
    const auto* m = master();
    return m ? m->name : "(?)";
}

ProfileCategory StockEntry::category() const {
    const auto* m = master();
    return m ? m->category : ProfileCategory::Unknown;
}

QString StockEntry::barcode() const {
    const auto* m = master();
    return m ? m->barcode : "(?)";
}

MaterialType StockEntry::type() const {
    const auto* m = master();
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}
