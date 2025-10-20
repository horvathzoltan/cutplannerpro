// #pragma once
// #include "auditstatus.h"
// #include "common/styleprofiles/auditcolors.h"
// #include <QColor>
// #include <QString>

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
