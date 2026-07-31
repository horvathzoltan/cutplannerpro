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
    { "toldas.safety_margin_mm",
        "Toldat ráhagyás [mm]",
        SettingType::Int,
        SettingCategory::Toldas,
        25,
        {}
    },

    { "toldas.max_main_shortfall_mm",
        "Fődarab max. rövidségi tolerancia [mm]",
        SettingType::Int,
        SettingCategory::Toldas,
        100,
        {}
    },
    { "toldas.max_main_overlength_mm",
        "Fődarab max. túlhossz tolerancia [mm]",
        SettingType::Int,
        SettingCategory::Toldas,
        100,
        {}
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


