#pragma once

#include <QString>

class CrossSectionShape {
public:
    enum class Shape {
        Round,
        Rectangular,
        Custom
    };

    Shape value;

    CrossSectionShape();                             // Default: Custom
    explicit CrossSectionShape(Shape s);             // Explicit konstruktor
    QString toString() const;                        // Enum → szöveg
    static CrossSectionShape fromString(const QString& str);  // Szöveg → enum
    bool operator==(const CrossSectionShape& other) const;

    // 💡 Később jöhet: isValid(diameter, width, height)
    // QColor uiColor() const;

    // 🔥 Kényelmi függvények
    inline bool isRectangular() const { return value == Shape::Rectangular; }
    inline bool isRound() const       { return value == Shape::Round; }
    inline bool isCustom() const      { return value == Shape::Custom; }
};
