// // AuditContext.cpp
#include "auditcontext.h"
#include "storageauditrow.h"

int AuditContext::confirmedCount() const
{
    int count = 0;
    for (const StorageAuditRow* row : _groupRows) {
        if (row->isRowAuditChecked || row->isRowModified)
            ++count;
    }
    return count;
}

[[nodiscard]] bool AuditContext::isGroupAudited() const {
    return std::any_of(_groupRows.begin(), _groupRows.end(), [](const StorageAuditRow* row) {
        return row && row->isAudited();
    });
}

[[nodiscard]] bool AuditContext::isGroupFulfilled() const {
    return std::all_of(_groupRows.begin(), _groupRows.end(), [](const StorageAuditRow* row) {
        return row && row->isFulfilled();
    });
}

[[nodiscard]] bool AuditContext::isGroupPartiallyAudited() const {
    int auditedCount = std::count_if(_groupRows.begin(), _groupRows.end(), [](const StorageAuditRow* row) {
        return row && row->isAudited();
    });
    return auditedCount > 0 && auditedCount < totalCount();
}
