#pragma once

#include <QString>

class StorageType {
public:
    enum class Type {
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


    bool isLocation() const
    {
        if(value == Type::Rack) return true;
        if(value == Type::Shelf) return true;
        if(value == Type::Box) return true;
        if(value == Type::Pallet) return true;
        if(value == Type::Crate) return true;
        if(value == Type::Floor) return true;

        return false;
    }
    // Opcionális UI helper
    // QColor uiColor() const;
};
