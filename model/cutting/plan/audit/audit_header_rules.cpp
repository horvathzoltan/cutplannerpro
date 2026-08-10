#include "audit_header_rules.h"

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

HeaderAuditResult AuditHeaderRules::check(const QVector<Cutting::Plan::Request>& list)
{
    HeaderAuditResult r;

    if (list.isEmpty())
        return r;

    const auto& first = list.first();

    // --- 1) Kötelező fejadatok ellenőrzése ---
    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "externalReference not empty",
        !first.externalReference.trimmed().isEmpty(),
        "non-empty",
        first.externalReference.trimmed()
        );


    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "ownerName not empty",
        !first.ownerName.trimmed().isEmpty(),
        "non-empty",
        first.ownerName.trimmed()
        );


    // dueDate
    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "dueDate is valid",
        first.dueDate.isValid(),
        "valid QDate",
        first.dueDate.toString("yyyy-MM-dd")
        );

    // productTypeId
    auto* type = ProductTypeRegistry::instance().findById(first.productTypeId);
    QString typeName = type ? type->code : first.productTypeId.toString();

    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "productTypeId not null",
        !first.productTypeId.isNull(),
        "non-null",
        typeName
        );

    // productSubtypeId
    auto* subtype = ProductSubtypeRegistry::instance().findById(first.productSubtypeId);
    QString subtypeName = subtype ? subtype->code : first.productSubtypeId.toString();

    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "productSubtypeId not null",
        !first.productSubtypeId.isNull(),
        "non-null",
        subtypeName
        );


    // quantity
    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "quantity > 0",
        first.quantity > 0,
        "> 0",
        QString::number(first.quantity)
        );

    // fullWidth_mm
    bool widthValid = (first.fullWidth_mm > 0);
    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "fullWidth_mm > 0",
        widthValid,
        "> 0",
        QString::number(first.fullWidth_mm)
        );
    if (!widthValid)
        r.hasValidDimensions = false;

    // fullHeight_mm
    bool heightValid = (first.fullHeight_mm > 0);
    r.entries.add(
        first.externalReference.trimmed(),
        "HEAD",
        "fullHeight_mm > 0",
        heightValid,
        "> 0",
        QString::number(first.fullHeight_mm)
        );
    if (!heightValid)
        r.hasValidDimensions = false;

    // --- 2) Tételszámon belüli konzisztencia ---
    for (const auto& req : list)
    {
        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "externalReference consistent",
            req.externalReference.trimmed() == first.externalReference.trimmed(),
            first.externalReference.trimmed(),
            req.externalReference.trimmed()
            );


        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "ownerName consistent",
            req.ownerName.trimmed() == first.ownerName.trimmed(),
            first.ownerName.trimmed(),
            req.ownerName.trimmed()
            );


        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "dueDate consistent",
            req.dueDate == first.dueDate,
            first.dueDate.toString("yyyy-MM-dd"),
            req.dueDate.toString("yyyy-MM-dd")
            );


        auto* typeFirst = ProductTypeRegistry::instance().findById(first.productTypeId);
        auto* typeReq   = ProductTypeRegistry::instance().findById(req.productTypeId);

        QString typeNameFirst = typeFirst ? typeFirst->code : first.productTypeId.toString();
        QString typeNameReq   = typeReq   ? typeReq->code   : req.productTypeId.toString();

        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "productTypeId consistent",
            req.productTypeId == first.productTypeId,
            typeNameFirst,
            typeNameReq
            );



        auto* subtypeFirst = ProductSubtypeRegistry::instance().findById(first.productSubtypeId);
        auto* subtypeReq   = ProductSubtypeRegistry::instance().findById(req.productSubtypeId);

        QString subtypeNameFirst = subtypeFirst ? subtypeFirst->code : first.productSubtypeId.toString();
        QString subtypeNameReq   = subtypeReq   ? subtypeReq->code   : req.productSubtypeId.toString();

        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "productSubtypeId consistent",
            req.productSubtypeId == first.productSubtypeId,
            subtypeNameFirst,
            subtypeNameReq
            );



        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "quantity consistent",
            req.quantity == first.quantity,
            QString::number(first.quantity),
            QString::number(req.quantity)
            );


        bool lrConsistent =
            req.leftCount  == first.leftCount &&
            req.rightCount == first.rightCount;

        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "left/right count consistent",
            lrConsistent,
            QString("%1 / %2").arg(first.leftCount).arg(first.rightCount),
            QString("%1 / %2").arg(req.leftCount).arg(req.rightCount)
            );

        bool widthConsistent = (req.fullWidth_mm == first.fullWidth_mm);

        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "fullWidth_mm consistent",
            widthConsistent,
            QString::number(first.fullWidth_mm),
            QString::number(req.fullWidth_mm)
            );

        if (!widthConsistent)
            r.hasValidDimensions = false;


        bool heightConsistent = (req.fullHeight_mm == first.fullHeight_mm);

        r.entries.add(
            first.externalReference.trimmed(),
            "HEAD",
            "fullHeight_mm consistent",
            heightConsistent,
            QString::number(first.fullHeight_mm),
            QString::number(req.fullHeight_mm)
            );

        if (!heightConsistent)
            r.hasValidDimensions = false;

        bool relConsistent = (req.relevantDim == first.relevantDim);

        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "relevantDim consistent",
            relConsistent,
            RelevantDimensionUtils::toString(first.relevantDim),
            RelevantDimensionUtils::toString(req.relevantDim)
            );

        if (!relConsistent)
            r.hasValidDimensions = false;


        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "surface consistent",
            req.surface == first.surface,
            SurfaceTypeUtils::toDisplayText(first.surface),
            SurfaceTypeUtils::toDisplayText(req.surface)
            );


        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "requiredColor consistent",
            req.requiredColor.code() == first.requiredColor.code(),
            first.requiredColor.code(),
            req.requiredColor.code()
            );


        r.entries.add(
            req.externalReference.trimmed(),
            "HEAD",
            "attributes consistent",
            req.attributes == first.attributes,
            first.attributesToString(),
            req.attributesToString()
            );

    }

    return r;
}
