#include "cuttingrequestregistry.h"

#include <common/filenamehelper.h>
#include <common/settingsmanager.h>
#include <model/repositories/cuttingrequestrepository.h>

CuttingRequestRegistry& CuttingRequestRegistry::instance() {
    // 🧵 Singleton implementáció: egyetlen példány az egész programban
    static CuttingRequestRegistry registry;
    return registry;
}

void CuttingRequestRegistry::persist() const {
    // 💾 Mentés fájlba, ha van megadott útvonal
    const QString fn = SettingsManager::instance().cuttingPlanFileName();
    const QString path = FileNameHelper::instance().getCuttingPlanFilePath(fn);

    if (!path.isEmpty())
        CuttingRequestRepository::saveToFile(*this, path);
}

QVector<CuttingRequest> CuttingRequestRegistry::readAll() const {
    // 📚 Visszaadja az összes CuttingRequest-et
    return _requests;
}

QVector<CuttingRequest> CuttingRequestRegistry::findByMaterialId(const QUuid& materialId) const {
    // 🧪 Lekérdezés anyagID alapján — bár a belső tárolás nem csoportosít, ez kiszűri
    QVector<CuttingRequest> result;
    for (const auto& r : _requests) {
        if (r.materialId == materialId)
            result.append(r);
    }
    return result;
}

void CuttingRequestRegistry::registerRequest(const CuttingRequest& request) {
    // 🆕 Új CuttingRequest hozzáadása
    _requests.append(request);
    persist();
}

bool CuttingRequestRegistry::updateRequest(const CuttingRequest& updated) {
    // 🔍 Érvényesség ellenőrzése
    if (!updated.isValid())
        return false;

    // 🔄 Megkeressük a megfelelő requestId-t a vektorban
    for (auto& r : _requests) {
        if (r.requestId == updated.requestId) {
            r = updated; // ✏️ Frissítés
            persist();   // 💾 Mentés
            return true;
        }
    }

    // ❌ Nem találtuk meg az adott requestId-t
    return false;
}

void CuttingRequestRegistry::removeRequest(const QUuid& requestId) {
    // 🗑️ Törlés egyedi azonosító alapján
    auto it = std::remove_if(_requests.begin(), _requests.end(),
                             [&](const CuttingRequest& r) {
                                 return r.requestId == requestId;
                             });

    if (it != _requests.end()) {
        _requests.erase(it, _requests.end());
        persist(); // 💾 Mentés csak akkor, ha történt törlés
    }
}

void CuttingRequestRegistry::clear() {
    // 🔄 Teljes lista törlése
    _requests.clear();
    persist();
}

std::optional<CuttingRequest> CuttingRequestRegistry::findById(const QUuid& requestId) const {
    for (const auto& r : _requests) {
        if (r.requestId == requestId)
            return r;
    }
    return std::nullopt;
}

