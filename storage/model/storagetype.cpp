#include "storagetype.h"

StorageType::StorageType() : value(Type::Unknown) {}
StorageType::StorageType(Type t) : value(t) {}

QString StorageType::toString() const {
    switch (value) {
    case Type::Warehouse: return "Warehouse";
    case Type::Shelf:     return "Shelf";
    case Type::Box:       return "Box";
    case Type::Pallet:    return "Pallet";
    case Type::Other:     return "Other";
    case Type::Unknown:   return "Unknown";
    }
    return "Unknown";
}

StorageType StorageType::fromString(const QString& str) {
    const QString lowered = str.toLower().trimmed();
    if (lowered == "warehouse") return StorageType(Type::Warehouse);
    if (lowered == "shelf")     return StorageType(Type::Shelf);
    if (lowered == "box")       return StorageType(Type::Box);
    if (lowered == "pallet")    return StorageType(Type::Pallet);
    if (lowered == "other")     return StorageType(Type::Other);
    return StorageType(Type::Unknown);
}

bool StorageType::operator==(const StorageType& other) const {
    return value == other.value;
}
