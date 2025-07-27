#include "settingsmanager.h"

#include "filenamehelper.h"

SettingsManager& SettingsManager::instance() {
    static SettingsManager _instance;
    return _instance;
}

SettingsManager::SettingsManager()
    : _settings(FileNameHelper::instance().getSettingsFilePath(), QSettings::IniFormat) {
    //load(); // inicializáláskor betöltjük
}



void SettingsManager::load() {
    // _cuttingPlan_FileName = _settings.value(SettingsKeys::CuttingPlanFileName).toString();
 }

void SettingsManager::save() {
//     _settings.setValue(SettingsKeys::CuttingPlanFileName, _cuttingPlan_FileName);
     _settings.sync(); // biztosítja az azonnali mentést
}

/*persist*/

void SettingsManager::persist(const QString &key, const QString &value) {
    _settings.setValue(key, value);
    _settings.sync();
}

void SettingsManager::persist(const QString& key, const QByteArray& value) {
    _settings.setValue(key, value);
    _settings.sync();
}

/*fn*/

QString SettingsManager::cuttingPlanFileName() const{
    return _settings.value(SettingsKeys::CuttingPlanFileName).toString();
}

void SettingsManager::setCuttingPlanFileName(const QString& fn) {
    //_cuttingPlan_FileName = fn;
    persist(SettingsKeys::CuttingPlanFileName, fn);
}


/*táblák*/

void SettingsManager::setInputTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableInputHeader, state);
}

QByteArray SettingsManager::inputTableHeaderState() const {
    return _settings.value(SettingsKeys::TableInputHeader).toByteArray();
}

void SettingsManager::setResultsTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableResultsHeader, state);
}

QByteArray SettingsManager::resultsTableHeaderState() const {
    return _settings.value(SettingsKeys::TableResultsHeader).toByteArray();
}

void SettingsManager::setStockTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableStockHeader, state);
}

QByteArray SettingsManager::stockTableHeaderState() const {
    return _settings.value(SettingsKeys::TableStockHeader).toByteArray();
}

void SettingsManager::setLeftoversTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableLeftoversHeader, state);
}

QByteArray SettingsManager::leftoversTableHeaderState() const {
    return _settings.value(SettingsKeys::TableLeftoversHeader).toByteArray();
}
