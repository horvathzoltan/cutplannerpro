#include "materialtype.h"

MaterialType::MaterialType() : value(Type::Unknown) {} // cpp-ben
MaterialType::MaterialType(Type t) : value(t) {}

QString MaterialType::toString() const {
    switch (value) {
    case Type::Aluminium: return "Aluminium";
    case Type::Steel: return "Steel";
    case Type::Plastic: return "Plastic";
    case Type::Composite: return "Composite";
    case Type::Other: return "Other";
    }
    return "Unknown";
}

MaterialType MaterialType::fromString(const QString& str) {
    const QString normalized = str.trimmed().toLower();

    if (normalized == "aluminium") return MaterialType(Type::Aluminium);
    if (normalized == "steel")     return MaterialType(Type::Steel);
    if (normalized == "plastic")   return MaterialType(Type::Plastic);
    if (normalized == "composite") return MaterialType(Type::Composite);
    if (normalized == "other") return MaterialType(Type::Other);
    return MaterialType(Type::Unknown);
}

bool MaterialType::operator==(const MaterialType& other) const {
    return this->value == other.value;
}
