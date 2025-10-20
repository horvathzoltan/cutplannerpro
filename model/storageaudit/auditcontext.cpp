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
