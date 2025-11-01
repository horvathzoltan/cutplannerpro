#pragma once

#include "common/styleprofiles/auditcolors.h"
//#include "model/storageaudit/auditcontext.h"
#include <QColor>
#include <QString>

class AuditStatus {
public:
    enum Value {
        Audited_Fulfilled,     // ✅ Auditált és teljesült
        Audited_Missing,       // 🟥 Auditált, de nincs készlet
        Audited_Unfulfilled,   // 🟡 Auditált, de részben teljesült
        Audited_Partial,       // 🟠 Részlegesen auditált csoport
        RegisteredOnly,        // 🔵 Regisztrált, nincs elvárás
        NotAudited             // ⚪ Még nem auditált
    };

    // Konstruktor
    explicit AuditStatus(Value v = NotAudited) : value(v) {}

    // Konverzió szövegre
    QString toText() const {
        switch (value) {
        case Audited_Fulfilled:    return "Auditált, teljesült";
        case Audited_Missing:      return "Auditált, nincs készlet";
        case Audited_Unfulfilled:  return "Auditált, részben teljesült";
        case Audited_Partial:      return "Részlegesen auditált";
        case RegisteredOnly:       return "Regisztrált (nincs elvárás)";
        case NotAudited:           return "Nem auditált";
        default:                   return "-";
        }
    }

    static QString statusEmoji(Value v) {
        switch (v) {
        case Audited_Fulfilled:    return "✅";
        case Audited_Missing:      return "🟥";
        case Audited_Unfulfilled:  return "🟡";
        case Audited_Partial:      return "🟠";
        case RegisteredOnly:       return "🔵";
        case NotAudited:           return "⚪";
        default:                   return "⚪";
        }
    }

    static QString toDecoratedText(Value v) {
        return statusEmoji(v) + " " + AuditStatus(v).toText();
    }

    // Konverzió színre
    QColor toColor() const {
        switch (value) {
        case Audited_Fulfilled:    return AuditColors::Ok;
        case Audited_Missing:      return AuditColors::Missing;
        case Audited_Unfulfilled:  return AuditColors::Pending;
        case Audited_Partial:      return AuditColors::PartialAudit;
        case RegisteredOnly:       return AuditColors::Info;
        case NotAudited:           return AuditColors::Unknown;
        default:                   return AuditColors::Unknown;
        }
    }

    // Getter az enum értékhez
    Value get() const { return value; }

    // Kényelmi operátorok
    operator Value() const { return value; }
    bool operator==(Value v) const { return value == v; }
    bool operator!=(Value v) const { return value != v; }



    QString toDecoratedText() const { return toDecoratedText(value); }

    // 🔹 Kiegészített szöveg (pl. módosítva, hulló, stb.)
    static QString withSuffix(Value v, const QString& suffix) {
        return toDecoratedText(v) + " " + suffix;
    }

    // 🔹 Standard suffixek
    static QString suffix_HulloVan()        { return "(hulló, van)"; }
    static QString suffix_HulloNincs()      { return "(hulló, nincs)"; }
    static QString suffix_HulloNemAudit()   { return "(hulló)"; }
    static QString suffix_Modositva()       { return "(módosítva)"; }
    static QString suffix_ModositvaNincs()  { return "(módosítva, nincs készlet)"; }
    static QString suffix_NincsKeszlet()    { return "(nincs készlet)"; }
    static QString suffix_ReszbenTeljesult() { return "(részben teljesült)"; }

    static QString suffix_CsoportReszbenTeljesult() { return "(csoport, részben teljesült)"; }
    static QString suffix_CsoportReszlegesAudit() { return "(csoport, részleges audit)"; }

private:
    Value value;
};

