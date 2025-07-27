#pragma once

#include <QSettings>

namespace SettingsKeys {
inline constexpr auto CuttingPlanFileName    = "cutting_plan_file_name";
inline constexpr auto TableInputHeader       = "table_input_header";
inline constexpr auto TableResultsHeader     = "table_results_header";
inline constexpr auto TableStockHeader       = "table_stock_header";
inline constexpr auto TableLeftoversHeader   = "table_leftovers_header";
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


    void save();
    void load();
private:
    SettingsManager();

    QSettings _settings;

    void persist(const QString& key, const QString& value);
    void persist(const QString &key, const QByteArray &value);
};

