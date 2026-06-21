#pragma once

#include <QStringList>
#include <QVariant>



enum class SettingType {
    Bool,
    Int,
    Enum,
    Action,     // reset gomb
    Separator
};

enum class SettingCategory {
    General,
    Cutting,
    MaterialFinder,
    Advanced
};

struct SettingMeta {
    QString key;
    QString label;
    SettingType type;
    SettingCategory category;
    QVariant defaultValue;
    QStringList enumValues; // ha type == Enum
};

#include <QVector>

extern const QVector<SettingMeta> SettingsMeta;
