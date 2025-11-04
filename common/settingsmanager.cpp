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



void SettingsManager::load(int argc, char* argv[]) {
    // _cuttingPlan_FileName = _settings.value(SettingsKeys::CuttingPlanFileName).toString();
        detectTestMode(argc, argv);
 }

void SettingsManager::detectTestMode(int argc, char* argv[]) {
    for (int i = 1; i < argc - 1; ++i) {
        QString arg = argv[i];
        QString next = argv[i + 1];
        if (arg == "--test") {
            _testProfile = next.toLower();
            return;
        }
    }

    // ini-ből
    QString testProfile = _settings.value("test").toString();
    if (!testProfile.isEmpty()) {
        _testProfile = testProfile.toLower();
        return;
    }

    if (_settings.value("testmode", false).toBool()) {
        _testProfile = "full";
    }
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
//CuttingPlanFileName
QString SettingsManager::cuttingPlanFileName() const{
    return _settings.value(SettingsKeys::CuttingPlanFileName).toString();
}

void SettingsManager::setCuttingPlanFileName(const QString& fn) {
    //_cuttingPlan_FileName = fn;
    persist(SettingsKeys::CuttingPlanFileName, fn);
}


/*táblák*/

//TableInputHeader
void SettingsManager::setInputTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableInputHeader, state);
}

QByteArray SettingsManager::inputTableHeaderState() const {
    return _settings.value(SettingsKeys::TableInputHeader).toByteArray();
}

//TableResultsHeader
void SettingsManager::setResultsTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableResultsHeader, state);
}

QByteArray SettingsManager::resultsTableHeaderState() const {
    return _settings.value(SettingsKeys::TableResultsHeader).toByteArray();
}

//TableStockHeader
void SettingsManager::setStockTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableStockHeader, state);
}

QByteArray SettingsManager::stockTableHeaderState() const {
    return _settings.value(SettingsKeys::TableStockHeader).toByteArray();
}

//TableLeftoversHeader
void SettingsManager::setLeftoversTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableLeftoversHeader, state);
}

QByteArray SettingsManager::leftoversTableHeaderState() const {
    return _settings.value(SettingsKeys::TableLeftoversHeader).toByteArray();
}

//TableStorageAuditHeader
void SettingsManager::setStorageAuditTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableStorageAuditHeader, state);
}

QByteArray SettingsManager::storageAuditTableHeaderState() const {
    return _settings.value(SettingsKeys::TableStorageAuditHeader).toByteArray();
}

//TableRelocationOrderHeader
void SettingsManager::setRelocationOrderTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::TableRelocationOrderHeader, state);
}

QByteArray SettingsManager::relocationOrderTableHeaderState() const {
    return _settings.value(SettingsKeys::TableRelocationOrderHeader).toByteArray();
}

//cuttingInstructionTableHeaderState
    void SettingsManager::setCuttingInstructionTableHeaderState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::CuttingInstructionTableHeader, state);
}

QByteArray SettingsManager::cuttingInstructionTableHeaderState() const {
    return _settings.value(SettingsKeys::CuttingInstructionTableHeader).toByteArray();
}

// ablakméret
void SettingsManager::setWindowGeometry(const QByteArray& state) {
    _settings.setValue(SettingsKeys::WindowGeometry, state);
}

QByteArray SettingsManager::windowGeometry() const {
    return _settings.value(SettingsKeys::WindowGeometry).toByteArray();
}

// splitter
void SettingsManager::setMainSplitterState(const QByteArray& state) {
    _settings.setValue(SettingsKeys::MainSplitterState, state);
}

QByteArray SettingsManager::mainSplitterState() const {
    return _settings.value(SettingsKeys::MainSplitterState).toByteArray();
}

// CuttingStrategy

void SettingsManager::setCuttingStrategy(Cutting::Optimizer::TargetHeuristic h) {
    // tároljuk stringként, hogy olvasható legyen az ini-ben
    QString value = (h == Cutting::Optimizer::TargetHeuristic::ByCount)
                        ? "ByCount"
                        : "ByTotalLength";
    persist(SettingsKeys::CuttingStrategy, value);
}

Cutting::Optimizer::TargetHeuristic SettingsManager::cuttingStrategy() const {
    QString value = _settings.value(SettingsKeys::CuttingStrategy, "ByCount").toString();
    if (value == "ByTotalLength") {
        return Cutting::Optimizer::TargetHeuristic::ByTotalLength;
    }
    return Cutting::Optimizer::TargetHeuristic::ByCount;
}

// optimizercounters

int SettingsManager::materialCounter() const { return _settings.value(SettingsKeys::MaterialCounter, 0).toInt(); }

int SettingsManager::leftoverCounter() const { return _settings.value(SettingsKeys::LeftoverCounter, 0).toInt(); }

int SettingsManager::nextMaterialCounter() {
    int c = materialCounter() + 1;
    persist(SettingsKeys::MaterialCounter, QString::number(c));
    return c;
}

int SettingsManager::nextLeftoverCounter() {
    int c = leftoverCounter() + 1;
    persist(SettingsKeys::LeftoverCounter, QString::number(c));
    return c;
}
