#pragma once

#include <QString>

class StorageType {
public:
    enum class Type {
        Warehouse,
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

    // Opcionális UI helper
    // QColor uiColor() const;
};
