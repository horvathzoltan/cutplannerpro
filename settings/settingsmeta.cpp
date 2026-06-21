#include "settingsmeta.h"
#include "settingsmanager.h"

const QVector<SettingMeta> SettingsMeta = {

// Cutting
{ SettingsKeys::CuttingStrategy,
    "Vágási stratégia",
    SettingType::Enum,
    SettingCategory::Cutting,
    "ByCount",
    { "ByCount", "ByTotalLength" }
},

    // MaterialFinder
    { SettingsKeys::MaterialFinderRange,
        "Anyagkereső range [mm]",
        SettingType::Int,
        SettingCategory::MaterialFinder,
        300,
        {}   // enumValues üres
    },

    // Leftovers
    { SettingsKeys::UseReusableLeftovers,
        "Újrahasználható maradékok",
        SettingType::Bool,
        SettingCategory::General,
        true,
        {}   // enumValues üres
    },

    // Separator
    { "",
        "",
        SettingType::Separator,
        SettingCategory::Advanced,
        {},
        {}   // enumValues üres
    },

    // Reset gombok
    { "reset_headers",
        "Táblafejek visszaállítása",
        SettingType::Action,
        SettingCategory::Advanced,
        {},
        {}   // enumValues üres
    },

    { "reset_window",
        "Ablakméret visszaállítása",
        SettingType::Action,
        SettingCategory::Advanced,
        {},
        {}   // enumValues üres
    },

{ "reset_counters",
        "Számlálók nullázása",
        SettingType::Action,
        SettingCategory::Advanced,
        {},
    {}   // enumValues üres
}
};

