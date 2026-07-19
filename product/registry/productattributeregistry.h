#pragma once
#include <QString>
#include <QMap>

class ProductAttributeRegistry
{
public:
    static ProductAttributeRegistry& instance()
    {
        static ProductAttributeRegistry inst;
        return inst;
    }

    // attribútum hozzáadása
    void add(const QString& productTypeId,
             const QString& productSubtypeId,
             const QString& key,
             const QString& defaultValue)
    {
        _map[productTypeId][productSubtypeId][key] = defaultValue;
    }

    // attribútum lekérdezése (először subtype, aztán wildcard)
    QString get(const QString& productTypeId,
                const QString& productSubtypeId,
                const QString& key) const
    {
        // subtype-specifikus
        if (_map.contains(productTypeId) &&
            _map[productTypeId].contains(productSubtypeId) &&
            _map[productTypeId][productSubtypeId].contains(key))
        {
            return _map[productTypeId][productSubtypeId][key];
        }

        // wildcard altípus
        if (_map.contains(productTypeId) &&
            _map[productTypeId].contains("*") &&
            _map[productTypeId]["*"].contains(key))
        {
            return _map[productTypeId]["*"][key];
        }

        return QString();
    }

    // összes attribútum lekérdezése egy típus+altípusra
    QMap<QString, QString> getAll(const QString& productTypeId,
                                  const QString& productSubtypeId) const
    {
        QMap<QString, QString> result;

        // subtype-specifikus attribútumok
        if (_map.contains(productTypeId) &&
            _map[productTypeId].contains(productSubtypeId))
        {
            for (auto it = _map[productTypeId][productSubtypeId].begin();
                 it != _map[productTypeId][productSubtypeId].end(); ++it)
            {
                result[it.key()] = it.value();
            }
        }

        // wildcard attribútumok (ha nincs felülírva)
        if (_map.contains(productTypeId) &&
            _map[productTypeId].contains("*"))
        {
            for (auto it = _map[productTypeId]["*"].begin();
                 it != _map[productTypeId]["*"].end(); ++it)
            {
                if (!result.contains(it.key()))
                    result[it.key()] = it.value();
            }
        }

        return result;
    }

    void initDefaults()
    {
        add("NP", "*", "meghajtas", "motoros");
    }

private:
    ProductAttributeRegistry() = default;

    // productTypeId → productSubtypeId → key → value
    QMap<QString, QMap<QString, QMap<QString, QString>>> _map;
};
