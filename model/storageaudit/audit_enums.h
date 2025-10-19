#pragma once

enum class AuditSourceType {
    Stock,
    Leftover
};

enum class AuditPresence {
    Unknown,
    Present,
    Missing
};

enum class AuditResult {
    NotAudited,   // nincs audit
    AuditedOk,    // auditált, rendben
   // AuditedMissing, // auditált, de hiányzik
    AuditedPartial // (csak csoportnál értelmezett)
};
