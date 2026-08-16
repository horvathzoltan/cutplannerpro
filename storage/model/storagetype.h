#pragma once

#include <QString>

class StorageType {
public:
    enum class Type {
        Site,
        Warehouse,
        Rack,
        Shelf,
        Box,
        Pallet,
        Floor,
        Zone,
        Crate,
        Other,
        Unknown
    };

    QString icon() const {
        switch (value) {
        case Type::Site: return "🏢"; // telephely / campus / facility
        case Type::Warehouse: return "📁";
        case Type::Rack:      return "📁";
        case Type::Shelf:     return "🗄️";
        case Type::Box:       return "📦";
        case Type::Pallet:    return "🗃️";
        case Type::Crate:     return "🗃️"; // kaloda
        case Type::Floor:     return "⬛";
        case Type::Zone:      return "⬛";
        case Type::Other:     return "📁";
        default:              return "📁";
        }
    }

    Type value;

    StorageType();                         // Default konstruktor → Unknown
    explicit StorageType(Type t);          // Enum alapú konstruktor
    QString toString() const;              // Enum → QString
    static StorageType fromString(const QString& str); // QString → Enum

    bool operator==(const StorageType& other) const; // Egyenlőség


    bool isNode() const
    {
        return value == Type::Site ||
               value == Type::Warehouse ||
               value == Type::Zone ||
               value == Type::Floor ||
               value == Type::Rack;
    }

    bool isLeaf() const
    {
        return value == Type::Shelf ||
               value == Type::Box ||
               value == Type::Pallet ||
               value == Type::Crate;
    }

    bool isLocation() const
    {
        return value == Type::Rack ||
               value == Type::Shelf ||
               value == Type::Box ||
               value == Type::Pallet ||
               value == Type::Crate ||
               value == Type::Floor;
    }


    // Opcionális UI helper
    // QColor uiColor() const;
};
