#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QList>

#include "product/model/product_calcmodes.h"

// 🔗 Terméktípus–altípus méretszámítási módok registry-je (singleton)
class ProductCalcModeRegistry
{
private:
    ProductCalcModeRegistry() = default;
    ProductCalcModeRegistry(const ProductCalcModeRegistry&) = delete;

    // kulcs: "TYPE;SUBTYPE"
    QMap<QString, ProductCalcModes> _data;

    QString makeKey(const QString& type, const QString& subtype) const;

public:
    static ProductCalcModeRegistry& instance();

    void clearAll();
    void registerEntry(const ProductCalcModes& entry);

    QVector<SizeCalcMode> getModes(const QString& typeCode,
                                   const QString& subtypeCode) const;

    SizeCalcMode getDefault(const QString& typeCode,
                            const QString& subtypeCode) const;

    QList<ProductCalcModes> readAll() const;

    bool isEmpty() const { return _data.isEmpty(); }

    void debugDump() const;
};
