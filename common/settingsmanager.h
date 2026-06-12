#pragma once

#include "../model/cutting/optimizer/targetheuristic.h"
#include <QSettings>

namespace SettingsKeys {
inline constexpr auto CuttingPlanFileName    = "cutting_plan_file_name";
// oszlopméret
inline constexpr auto TableInputHeader       = "table_input_header";
inline constexpr auto TableResultsHeader     = "table_results_header";
inline constexpr auto TableStockHeader       = "table_stock_header";
inline constexpr auto TableLeftoversHeader   = "table_leftovers_header";
inline constexpr auto TableStorageAuditHeader   = "table_storageaudit_header";
inline constexpr auto TableRelocationOrderHeader   = "table_relocationorder_header";
inline constexpr auto CuttingInstructionTableHeader   = "table_cuttinginstruction_header";

// ablakméret
inline constexpr auto WindowGeometry = "window_geometry";
// splitter
inline constexpr auto MainSplitterState = "main_splitter_state";

// CuttingStrategy
inline constexpr auto CuttingStrategy = "cutting_strategy";

// optimize számlálók
inline constexpr auto MaterialCounter = "material_counter";
inline constexpr auto LeftoverCounter = "leftover_counter";
inline constexpr auto ManualLeftoverCounter = "manual_leftover_counter";

inline constexpr auto LastActiveTab = "last_active_tab";

inline constexpr auto UseReusableLeftovers = "use_reusable_leftovers";
}

enum class TestMode {
    None,
    Maki,
    Full
};

class SettingsManager {
public:
    static SettingsManager& instance();


    QString cuttingPlanFileName() const;
    void setCuttingPlanFileName(const QString& fn);

    // Input tábla
    void setInputTableHeaderState(const QByteArray& state);
    QByteArray inputTableHeaderState() const;

    // Results tábla
    void setResultsTableHeaderState(const QByteArray& state);
    QByteArray resultsTableHeaderState() const;

    // Stock tábla
    void setStockTableHeaderState(const QByteArray& state);
    QByteArray stockTableHeaderState() const;

    // Leftovers tábla
    void setLeftoversTableHeaderState(const QByteArray& state);
    QByteArray leftoversTableHeaderState() const;

    // storage audit tábla
    void setStorageAuditTableHeaderState(const QByteArray &state);
    QByteArray storageAuditTableHeaderState() const;

    // RelocationOrder tábla
    void setRelocationOrderTableHeaderState(const QByteArray &state);
    QByteArray relocationOrderTableHeaderState() const;


    // cuttingInstructionTableHeaderState
    void setCuttingInstructionTableHeaderState(const QByteArray &state);
    QByteArray cuttingInstructionTableHeaderState() const;

    // ablakmléret
    void setWindowGeometry(const QByteArray& state);
    QByteArray windowGeometry() const;

    //splitter
    void setMainSplitterState(const QByteArray& state);
    QByteArray mainSplitterState() const;

    // CuttingStrategy
    void setCuttingStrategy(Cutting::Optimizer::TargetHeuristic h);
    Cutting::Optimizer::TargetHeuristic cuttingStrategy() const;

    // useReusableLeftovers
    void setUseReusableLeftovers(bool enabled);
    bool useReusableLeftovers() const;

    int lastActiveTab() const;
    void setLastActiveTab(int ix);

    // optimizercounters
    int materialCounter() const;
    int leftoverCounter() const;
    int nextMaterialCounter();
    int nextLeftoverCounter();

    int manualLeftoverCounter() const;
    int peekManualLeftoverCounter() const; // shadow counter lekérdezés
    int nextManualLeftoverCounter();
    void commitManualLeftoverCounter(int c);

    //void setManualLeftoverCounter(int value);

    void save();
    void load(int argc, char* argv[]);

    QString testProfile() const { return _testProfile; }

    bool isTestMode(){
        if(_testProfile.isEmpty()) return false;
        if(_testProfile.toLower() == "none") return false;
        return true;
    }

private:
    SettingsManager();

    QSettings _settings;
    QString _testProfile = "none";

    void persist(const QString& key, const QString& value);
    void persist(const QString &key, const QByteArray &value);

    void detectTestMode(int argc, char *argv[]);
};


