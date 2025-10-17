#pragma once

#include "model/cutting/optimizer/optimizermodel.h"
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

// ablakméret
inline constexpr auto WindowGeometry = "window_geometry";
// splitter
inline constexpr auto MainSplitterState = "main_splitter_state";

// CuttingStrategy
inline constexpr auto CuttingStrategy = "cutting_strategy";
}


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

    // ablakmléret
    void setWindowGeometry(const QByteArray& state);
    QByteArray windowGeometry() const;

    //splitter
    void setMainSplitterState(const QByteArray& state);
    QByteArray mainSplitterState() const;

    // CuttingStrategy
    void setCuttingStrategy(Cutting::Optimizer::TargetHeuristic h);
    Cutting::Optimizer::TargetHeuristic cuttingStrategy() const;

    void save();
    void load();
private:
    SettingsManager();

    QSettings _settings;

    void persist(const QString& key, const QString& value);
    void persist(const QString &key, const QByteArray &value);
};

