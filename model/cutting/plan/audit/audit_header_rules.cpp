#include "audit_header_rules.h"

HeaderAuditResult AuditHeaderRules::check(const QVector<Cutting::Plan::Request>& list)
{
    HeaderAuditResult r;

    if (list.isEmpty())
        return r;

    const auto& first = list.first();

    // --- 1) Kötelező fejadatok ellenőrzése ---
    if (first.externalReference.trimmed().isEmpty()) {
        r.messages << "❌ Hiányzó tételszám (externalReference)";
        r.hasError = true;
    }

    if (first.ownerName.trimmed().isEmpty()) {
        r.messages << "❌ Hiányzó megrendelő név (ownerName)";
        r.hasError = true;
    }

    if (!first.dueDate.isValid()) {
        r.messages << "❌ Hiányzó vagy érvénytelen határidő (dueDate)";
        r.hasError = true;
    }

    if (first.productTypeId.isNull()) {
        r.messages << "❌ Hiányzó terméktípus (productTypeId)";
        r.hasError = true;
    }

    if (first.productSubtypeId.isNull()) {
        r.messages << "❌ Hiányzó altípus (productSubtypeId)";
        r.hasError = true;
    }

    if (first.quantity <= 0) {
        r.messages << "❌ Hiányzó vagy érvénytelen darabszám (quantity)";
        r.hasError = true;
    }

    if (first.fullWidth_mm <= 0) {
        r.messages << "⚠️ Hiányzó vagy érvénytelen teljes szélesség (fullWidth_mm)";
        r.hasError = true;
        r.hasValidDimensions = false;
    }

    if (first.fullHeight_mm <= 0) {
        r.messages << "⚠️ Hiányzó vagy érvénytelen teljes magasság (fullHeight_mm)";
        r.hasError = true;
        r.hasValidDimensions = false;
    }

    // --- 2) Tételszámon belüli konzisztencia ---
    for (const auto& req : list)
    {
        if (req.externalReference.trimmed() != first.externalReference.trimmed()) {
            r.messages << "❌ Tételszám eltérés egy tételszámon belül";
            r.hasError = true;
        }

        if (req.ownerName.trimmed() != first.ownerName.trimmed()) {
            r.messages << QString("❌ Megrendelő eltérés: %1 ↔ %2")
                              .arg(req.ownerName).arg(first.ownerName);
            r.hasError = true;
        }

        if (req.dueDate != first.dueDate) {
            r.messages << QString("❌ Határidő eltérés: %1 ↔ %2")
                              .arg(req.dueDate.toString("yyyy-MM-dd"))
                              .arg(first.dueDate.toString("yyyy-MM-dd"));
            r.hasError = true;
        }

        if (req.productTypeId != first.productTypeId) {
            r.messages << "❌ Terméktípus eltérés egy tételszámon belül";
            r.hasError = true;
        }

        if (req.productSubtypeId != first.productSubtypeId) {
            r.messages << "❌ Altípus eltérés egy tételszámon belül";
            r.hasError = true;
        }

        if (req.quantity != first.quantity) {
            r.messages << QString("❌ Darabszám eltérés: %1 ↔ %2")
                              .arg(req.quantity).arg(first.quantity);
            r.hasError = true;
        }

        if (req.leftCount != first.leftCount ||
            req.rightCount != first.rightCount)
        {
            r.messages << "❌ Balos/jobbos darabszám eltérés egy tételszámon belül";
            r.hasError = true;
        }

        if (req.fullWidth_mm != first.fullWidth_mm) {
            r.messages << QString("❌ Szélesség eltérés: %1 ↔ %2")
                              .arg(req.fullWidth_mm).arg(first.fullWidth_mm);
            r.hasError = true;
            r.hasValidDimensions = false;
        }

        if (req.fullHeight_mm != first.fullHeight_mm) {
            r.messages << QString("❌ Magasság eltérés: %1 ↔ %2")
                              .arg(req.fullHeight_mm).arg(first.fullHeight_mm);
            r.hasError = true;
            r.hasValidDimensions = false;
        }

        if (req.relevantDim != first.relevantDim) {
            r.messages << "❌ Releváns dimenzió eltérés egy tételszámon belül";
            r.hasError = true;
            r.hasValidDimensions = false;
        }

        if (req.surface != first.surface) {
            r.messages << "❌ Felületkezelés eltérés egy tételszámon belül";
            r.hasError = true;
        }

        if (req.requiredColor.code() != first.requiredColor.code()) {
            r.messages << "❌ Szín eltérés egy tételszámon belül";
            r.hasError = true;
        }

        if (req.attributes != first.attributes) {
            r.messages << "❌ Attribútum eltérés egy tételszámon belül";
            r.hasError = true;
        }
    }

    return r;
}
