#pragma once

#include <QStringList>
#include <model/cutting/plan/request.h>
#include <model/registries/materialregistry.h>
#include <model/material/material_utils.h>

//namespace Service {
namespace Cutting {
namespace Plan {

QString requestToDisplay(const Cutting::Plan::Request& r, const QChar& sep = '|') {
    QStringList parts;

    // 🧾 Külső hivatkozás
    if (!r.externalReference.isEmpty())
        parts << QString("Azonosító: \"%1\"").arg(r.externalReference);

    // 👤 Megrendelő neve
    if (!r.ownerName.isEmpty())
        parts << QString("Megrendelő: \"%1\"").arg(r.ownerName);

    // 📏 Hossz és mennyiség
    parts << QString("%1 mm × %2 db").arg(r.requiredLength).arg(r.quantity);

    // 🔗 Anyag UUID (mindig legyen benne)
    MaterialUtils::materialToDisplay(r.materialId);

    // 💡 Végső összefűzés
    return parts.join(sep);
}

} // endof namespace Plan
} // endof namespace Cutting
//} // endof namespace Service
