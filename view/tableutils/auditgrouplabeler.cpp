#include "view/tableutils/auditgrouplabeler.h"

namespace TableUtils {

QString AuditGroupLabeler::labelFor(const AuditContext* ctx) {
    if (!ctx)
        return "";

    const AuditGroupInfo& group = ctx->group;

    if (!group.isGroup())
        return ""; // 🔹 nincs címke egyelemű csoportokra

    const QString& key = group.groupKey();
    if (_labels.contains(key))
        return _labels.value(key);

    QString label = QString(QChar('A' + static_cast<int>(_labels.size())));
    _labels[key] = label;
    return label;
}


void AuditGroupLabeler::clear() {
    _labels.clear();
}

} // namespace TableUtils
