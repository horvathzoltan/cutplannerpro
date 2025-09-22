#include "common/tableutils/auditgrouplabeler.h"

namespace TableUtils {

QString AuditGroupLabeler::labelFor(const AuditContext* ctx) {
    if (!ctx)
        return "";

    if (_labels.contains(ctx))
        return _labels.value(ctx);

    // 🔠 Új betűjel generálása: A, B, C, ...
    QString label = QString(QChar('A' + static_cast<int>(_labels.size())));
    _labels[ctx] = label;
    return label;
}


void AuditGroupLabeler::clear() {
    _labels.clear();
}

} // namespace TableUtils
