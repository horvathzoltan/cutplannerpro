// common/auditcontextbuilder.h
#pragma once

#include <QHash>
#include <memory>
#include "model/storageaudit/auditcontext.h"
#include "model/storageaudit/storageauditrow.h"

class AuditContextBuilder {
public:
    // Map: rowId -> shared_ptr<AuditContext>
    static QHash<QUuid, std::shared_ptr<AuditContext>>
    buildFromRows(const QList<StorageAuditRow>& rows, const QMap<QUuid,int>& requiredStockMaterials);
    static QString makeGroupKey(const StorageAuditRow& r);

private:
    static const bool _isVerbose = false;
};
