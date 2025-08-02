#include "cuttingplanrequestregistry.h"

#include <common/filenamehelper.h>
#include <common/settingsmanager.h>
#include <model/repositories/cuttingrequestrepository.h>

CuttingPlanRequestRegistry& CuttingPlanRequestRegistry::instance() {
    // 🧵 Singleton implementáció: egyetlen példány az egész programban
    static CuttingPlanRequestRegistry registry;
    return registry;
}

void CuttingPlanRequestRegistry::persist() const {
    // 💾 Mentés fájlba, ha van megadott útvonal
    const QString fn = SettingsManager::instance().cuttingPlanFileName();
    const QString path = FileNameHelper::instance().getCuttingPlanFilePath(fn);

    if (!path.isEmpty())
        CuttingRequestRepository::saveToFile(*this, path);
}

QVector<CuttingPlanRequest> CuttingPlanRequestRegistry::readAll() const {
    // 📚 Visszaadja az összes CuttingRequest-et
    return _data;
}

QVector<CuttingPlanRequest> CuttingPlanRequestRegistry::findByMaterialId(const QUuid& materialId) const {
    // 🧪 Lekérdezés anyagID alapján — bár a belső tárolás nem csoportosít, ez kiszűri
    QVector<CuttingPlanRequest> result;
    for (const auto& r : _data) {
        if (r.materialId == materialId)
            result.append(r);
    }
    return result;
}

void CuttingPlanRequestRegistry::registerRequest(const CuttingPlanRequest& request) {
    // 🆕 Új CuttingRequest hozzáadása
    _data.append(request);
    persist();
}

bool CuttingPlanRequestRegistry::updateRequest(const CuttingPlanRequest& updated) {
    // 🔍 Érvényesség ellenőrzése
    if (!updated.isValid())
        return false;

    // 🔄 Megkeressük a megfelelő requestId-t a vektorban
    for (auto& r : _data) {
        if (r.requestId == updated.requestId) {
            r = updated; // ✏️ Frissítés
            persist();   // 💾 Mentés
            return true;
        }
    }

    // ❌ Nem találtuk meg az adott requestId-t
    return false;
}

void CuttingPlanRequestRegistry::removeRequest(const QUuid& requestId) {
    // 🗑️ Törlés egyedi azonosító alapján
    auto it = std::remove_if(_data.begin(), _data.end(),
                             [&](const CuttingPlanRequest& r) {
                                 return r.requestId == requestId;
                             });

    if (it != _data.end()) {
        _data.erase(it, _data.end());
        persist(); // 💾 Mentés csak akkor, ha történt törlés
    }
}

void CuttingPlanRequestRegistry::clear() {
    // 🔄 Teljes lista törlése
    _data.clear();
    persist();
}

std::optional<CuttingPlanRequest> CuttingPlanRequestRegistry::findById(const QUuid& requestId) const {
    for (const auto& r : _data) {
        if (r.requestId == requestId)
            return r;
    }
    return std::nullopt;
}

