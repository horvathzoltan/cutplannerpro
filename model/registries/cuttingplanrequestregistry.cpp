#include "cuttingplanrequestregistry.h"

#include <common/filenamehelper.h>
#include <common/settingsmanager.h>
#include <model/repositories/cuttingrequestrepository.h>
#include <QDebug>

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

QVector<Cutting::Plan::Request> CuttingPlanRequestRegistry::readAll() const {
    // 📚 Visszaadja az összes CuttingRequest-et
    return _data;
}

// QVector<Request> CuttingPlanRequestRegistry::findByMaterialId(const QUuid& materialId) const {
//     // 🧪 Lekérdezés anyagID alapján — bár a belső tárolás nem csoportosít, ez kiszűri
//     QVector<Request> result;
//     for (const auto& r : _data) {
//         if (r.materialId == materialId)
//             result.append(r);
//     }
//     return result;
// }

void CuttingPlanRequestRegistry::registerRequest(const Cutting::Plan::Request& request) {
    // 🆕 Új CuttingRequest hozzáadása
    _data.append(request);
    persist();
}

// void CuttingPlanRequestRegistry::registerRequest_NotPersistant(const Request& request) {
//     // 🆕 Új CuttingRequest hozzáadása
//     _data.append(request);
// }

bool CuttingPlanRequestRegistry::updateRequest(const Cutting::Plan::Request& updated) {
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
                             [&](const Cutting::Plan::Request& r) {
                                 return r.requestId == requestId;
                             });

    if (it != _data.end()) {
        _data.erase(it, _data.end());
        persist(); // 💾 Mentés csak akkor, ha történt törlés
    }
}

void CuttingPlanRequestRegistry::clearAll() {
    // 🔄 Teljes lista törlése
    _data.clear();
    persist();
}

Cutting::Plan::Request* CuttingPlanRequestRegistry::findById(const QUuid& requestId) {
    for (auto& r : _data) {
        if (r.requestId == requestId)
            return &r;
    }
    return nullptr;
}

