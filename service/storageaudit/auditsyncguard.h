#pragma once

#include "auditstatemanager.h"

class AuditSyncGuard {
public:
    AuditSyncGuard(AuditStateManager* manager)
        : _manager(manager), _wasTracking(manager && manager->isTrackingEnabled()) {
        if (_manager)
            _manager->setTrackingEnabled(false);
    }

    ~AuditSyncGuard() {
        if (_manager && _wasTracking)
            _manager->setTrackingEnabled(true);
    }

private:
    AuditStateManager* _manager = nullptr;
    bool _wasTracking = false;
};
