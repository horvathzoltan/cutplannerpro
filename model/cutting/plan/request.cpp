#include "request.h"
#include <QRegularExpression>

namespace Cutting {
namespace Plan {
QStringList Request::invalidReasons() const {
    QStringList errors;

    if (materialId.isNull())
        errors << "• Anyag ID hiányzik vagy érvénytelen.";

    if (requiredLength <= 0)
        errors << "• A vágáshossz nem lehet nulla vagy negatív.";

    if (quantity <= 0)
        errors << "• A darabszám nem lehet nulla vagy negatív.";

    if (quantity > 500)
        errors << "• A darabszám túl magas (max. 500).";

    if (ownerName.trimmed().isEmpty())
        errors << "• A megrendelő neve nem lett megadva.";

    if (externalReference.trimmed().isEmpty())
        errors << "• A külső hivatkozás nem lehet üres.";

    if (externalReference.length() > 64)
        errors << "• A külső hivatkozás túl hosszú (max. 64 karakter).";

    QRegularExpression unsafe("[\"';]+");
    if (unsafe.match(externalReference).hasMatch())
        errors << "• A külső hivatkozás veszélyes karaktert tartalmaz.";


    return errors;
}

bool Request::isValid() const {
    return invalidReasons().isEmpty();
}

QString Request::toString() const {
    QStringList parts;

    // 🧾 Külső hivatkozás
    if (!externalReference.isEmpty())
        parts << QString("Azonosító: \"%1\"").arg(externalReference);

    // 👤 Megrendelő neve
    if (!ownerName.isEmpty())
        parts << QString("Megrendelő: \"%1\"").arg(ownerName);

    // 📏 Hossz és mennyiség
    parts << QString("%1 mm × %2 db").arg(requiredLength).arg(quantity);

    // 🔗 Anyag UUID (mindig legyen benne)
    parts << QString("Anyag ID: %1").arg(materialId.toString());

    // 💡 Végső összefűzés
    return parts.join(" | ");
}

QString Request::displayText()
{
    return QString("%1 • %2 • %3 mm")
        .arg(ownerName)
        .arg(externalReference)
        .arg(requiredLength);
}


} //endof namespace Plan
} //endof namespace Cutting
