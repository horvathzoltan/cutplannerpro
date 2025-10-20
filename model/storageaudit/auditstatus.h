#pragma once

#include "common/styleprofiles/auditcolors.h"
#include "model/storageaudit/audit_enums.h"
#include <QColor>
#include <QString>


class AuditStatus {
public:
    enum Value {
        Ok,       // elvárt teljesül
        Missing,  // semmi nincs jelen
        Pending,  // részben jelen, de nem elég
        Info,     // nincs elvárás (optimize előtt, vagy csak információ)
        Unknown
    };

    // Konstruktor
    explicit AuditStatus(Value v = Unknown) : value(v) {}

    // Konverzió szövegre
    QString toText() const {
        switch (value) {
        case Ok:      return "OK";
        case Missing: return "Hiányzik";
        case Pending: return "Ellenőrzésre vár";
        case Info:    return "Regisztrált";
        default:      return "-";
        }
    }

    // Konverzió színre
    QColor toColor() const {
        switch (value) {
        case Ok:      return AuditColors::Ok;       // zöld
        case Missing: return AuditColors::Missing;  // piros
        case Pending: return AuditColors::Pending;  // sárga/narancs
        case Info:    return AuditColors::Info;     // kékes/szürke
        default:      return AuditColors::Unknown;  // szürke
        }
    }

    // Getter az enum értékhez
    Value get() const { return value; }

    // Kényelmi operátorok
    operator Value() const { return value; }
    bool operator==(Value v) const { return value == v; }
    bool operator!=(Value v) const { return value != v; }


    // 🔹 AuditPresence → AuditStatus
    static AuditStatus fromPresence(AuditPresence presence) {
        switch (presence) {
        case AuditPresence::Unknown: return AuditStatus(Unknown);
        case AuditPresence::Missing: return AuditStatus(Pending);
        case AuditPresence::Present: return AuditStatus(Ok);
        }
        return AuditStatus(Unknown);
    }

    // 🔹 Csoportos státusz szöveg
    static QString fromPresenceText(AuditPresence presence) {
        switch (presence) {
        case AuditPresence::Unknown: return "⚪ Nem auditált (csoport)";
        case AuditPresence::Missing: return "🟡 Részlegesen auditált (csoport)";
        case AuditPresence::Present: return "🟢 Auditálva (csoport)";
        }
        return "⚪ Nem auditált (csoport)";
    }

    static QString toDecoratedText(Value v) {
        switch (v) {
        case Ok:      return "🟢 Auditált";
        case Missing: return "🟠 Auditált (nincs készlet)";
        case Pending: return "🟡 Auditált (részleges)";
        case Info:    return "🔵 Regisztrált";
        default:      return "⚪ Nem auditált";
        }
    }

    QString toDecoratedText() const { return toDecoratedText(value); }

    // 🔹 Kiegészített szöveg (pl. módosítva, hulló, stb.)
    static QString withSuffix(Value v, const QString& suffix) {
        return toDecoratedText(v) + " " + suffix;
    }

    // 🔹 Standard suffixek
    static QString suffixHullóVan()        { return QStringLiteral("(hulló, van)"); }
    static QString suffixHullóNincs()      { return "(hulló, nincs)"; }
    static QString suffixHullóNemAudit()   { return "(hulló)"; }
    static QString suffixMódosítva()       { return "(módosítva)"; }
    static QString suffixMódosítvaNincs()  { return "(módosítva, nincs készlet)"; }
    static QString suffixNincsKészlet()    { return "(nincs készlet)"; }
private:
    Value value;
};

// enum AuditStatus {
//     Ok,       // elvárt teljesül
//     Missing,  // semmi nincs jelen
//     Pending,  // részben jelen, de nem elég
//     Info,     // nincs elvárás (optimize előtt, vagy csak információ)
//     Unknown
// };

// namespace StorageAudit{
// namespace Status{

// inline QString toText(AuditStatus s) {
//     switch (s) {
//     case AuditStatus::Ok:      return "OK";
//     case AuditStatus::Missing: return "Hiányzik";
//     case AuditStatus::Pending: return "Ellenőrzésre vár";
//     case AuditStatus::Info:    return "Regisztrált";
//     default:                   return "-";
//     }
// }

// inline QColor toColor(AuditStatus s) {
//     switch (s) {
//     case AuditStatus::Ok:      return AuditColors::Ok; // zöld
//     case AuditStatus::Missing: return AuditColors::Missing; // piros
//     case AuditStatus::Pending: return AuditColors::Pending; // narancs
//     case AuditStatus::Info:    return AuditColors::Info;
//     default:                   return AuditColors::Unknown;
//     }
// }


// }
// }
