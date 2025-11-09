#pragma once

#include <QString>
#include <QUuid>

// 🔷 Alapstruktúra az összes azonosítható törzselemhez:
// - Minden olyan entitás, ami rendelkezik technikai ID-val, névvel, vonalkóddal
// - Alkalmas törzsregiszterekhez, UI megjelenítéshez, címkézéshez

struct IdentifiableEntity {
    QUuid id;           // 🆔 Technikai, rendszer által generált egyedi azonosító (általában GUID)
    QString name;       // 📛 Emberbarát megnevezés, pl. UI-ban megjelenő cím
    QString barcode;    // 🏷️ Nyomtatott, beolvasott vagy egyéb fizikai kód (általában "MAT-..." vagy egyedi string)

    // 🖼️ Vizuális, felhasználóbarát megjelenítési név (pl. listában)
    QString toDisplay() const {
        QString suffix;
        if (!barcode.isEmpty()) suffix += "[" + barcode + "]";
        return suffix.isEmpty() ? name : name + " " + suffix;
    }

    // 🧾 Teljes technikai szöveges reprezentáció – log, export, debug célra
    QString toString() const {
        return QString("IdentifiableEntity{id=%1, name=%2, barcode=%3}")
        .arg(id.toString(), name, barcode);
    }

    // 🏷️ Opcionális címkekód (pl. barcode, de később override-olható)
    QString labelCode() const {
        return barcode;
    }

    // 🔢 Rövidített ID (pl. QR vagy logokban csak az első 8 karakter)
    QString shortId() const {
        return id.toString(QUuid::WithoutBraces).left(8);
    }
};
