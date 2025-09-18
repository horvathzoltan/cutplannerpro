#pragma once

enum AuditStatus {
    Ok,       // elvárt teljesül
    Missing,  // semmi nincs jelen
    Pending,  // részben jelen, de nem elég
    Info,     // nincs elvárás (optimize előtt, vagy csak információ)
    Unknown
};
