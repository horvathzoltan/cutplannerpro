#pragma once
#include <optional>
#include <QString>
#include <QMap>
#include "calcmode.h"

class LengthCalculator
{
public:
    static std::optional<double> calculate(
        const QString& productType,
        const QString& productSubtype,
        const QMap<QString, QString>& attributes,
        const QString& role,
        double width,
        double height,
        CalcMode mode);

    static std::optional<double> compensate(
        const QString& productType,
        const QString& productSubtype,
        const QMap<QString, QString>& attributes,
        const QString& role);
};
