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
        Other,
        Unknown
    };

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

        return false;
    }
    // Opcionális UI helper
    // QColor uiColor() const;
};
