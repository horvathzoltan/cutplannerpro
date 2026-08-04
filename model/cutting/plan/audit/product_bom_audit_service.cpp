#include "product_bom_audit_service.h"

#include "common/logger.h"
#include "audit_header_rules.h"
#include "audit_length_rules.h"
#include "product/utils/material_role_utils.h"
#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>
#include <materials/registry/material_registry.h>

#include <materials/model/material_family_utils.h>

#include <product/registry/product_subtype_registry.h>
#include <product/registry/product_type_registry.h>

#include <model/registries/cuttingplanrequestregistry.h>


// product_bom_audit_service.cpp

QHash<QString, QVector<Cutting::Plan::Request>>
ProductBomAuditService::groupByExternalRef(const QVector<Cutting::Plan::Request>& all)
{
    QHash<QString, QVector<Cutting::Plan::Request>> groups;

    for (const auto& req : all) {
        QString ref = req.externalReference.trimmed();
        if (ref.isEmpty())
            ref = "<NINCS_TETELSZAM>";
        groups[ref].append(req);
    }

    return groups;
}

ProductBomAuditResult ProductBomAuditService::run(const QVector<Cutting::Plan::Request>& all)
{
    ProductBomAuditResult result;

    // 1) Tételszámok csoportosítása
    auto groups = groupByExternalRef(all);

    for (auto it = groups.begin(); it != groups.end(); ++it)
    {
        const QString ref = it.key();
        const auto& list = it.value();

        if (list.isEmpty())
            continue;

        // 2) Típus/altípus konzisztencia
        const QUuid typeId    = list.first().productTypeId;
        const QUuid subtypeId = list.first().productSubtypeId;

        bool typeMismatch = false;
        for (const auto& r : list) {
            if (r.productTypeId != typeId || r.productSubtypeId != subtypeId) {
                typeMismatch = true;
                break;
            }
        }

        auto* type = ProductTypeRegistry::instance().findById(typeId);
        auto* subtype = ProductSubtypeRegistry::instance().findById(subtypeId);

        QString typeName    = type    ? type->code    : typeId.toString();
        QString subtypeName = subtype ? subtype->code : subtypeId.toString();

        QString expectedName = QString("%1 / %2").arg(typeName, subtypeName);

        if (typeMismatch) {
            result.entries.add(
                ref,
                "BOM",
                "type/subtype consistent",
                false,
                expectedName,
                "mismatch in list"
                );
        }

        // 2/a) HEAD audit
        auto head = AuditHeaderRules::check(list);
        result.entries.addAll(head.entries.readAll());

        // 2/b) LENGTH audit
        auto len = AuditLengthRules::check(list, typeId, subtypeId);
        result.entries.addAll(len.entries.readAll());

        // 3) BOM lekérése
        auto bomMap = BomRegistry::instance().bomMap(typeId, subtypeId);

        if (bomMap.isEmpty()) {
            result.entries.add(
                ref,
                "BOM",
                "bom exists",
                false,
                "non-empty BOM",
                "empty BOM"
                );
            continue;
        }

        // 4) Request anyagok aggregálása role‑alapú family szerint
        QHash<QString, double> actual;

        for (const auto& r : list)
        {
            const auto* mm = MaterialRegistry::instance().findById(r.materialId);
            QString matName = mm ? mm->barcode : r.materialId.toString();
            if (!mm) {
                result.entries.add(
                    ref,
                    "BOM",
                    "material exists",
                    false,
                    "known materialId",
                    mm->toReportLabel()
                    );
                continue;
            }

            MaterialRole normalized = MaterialRoleUtils::makeRole(r, mm);

            if (normalized.family == MaterialFamily::Unknown ||
                normalized.barcodePrefix.isEmpty())
            {
                result.entries.add(
                    ref,
                    "BOM",
                    "role known",
                    false,
                    "valid role",
                    mm->barcode
                    );
                continue;
            }

            QString roleKey = normalized.barcodePrefix.trimmed();
            actual[roleKey] += r.quantity;
        }

        auto bomRoleMap = BomRegistry::instance().bomRoleMap(typeId, subtypeId);

        int productCount = list.first().quantity;

        // 5) BOM hiány / többlet
        for (auto it2 = bomRoleMap.begin(); it2 != bomRoleMap.end(); ++it2)
        {
            QString rawKey = it2.key();
            QString roleKey = MaterialRoleUtils::normalizePrefix(rawKey);

            double expectedPerProduct = it2.value();
            double expected = expectedPerProduct * productCount;

            double got = actual.value(roleKey, 0.0);

            bool ok = (got == expected);

            result.entries.add(
                ref,
                "BOM",
                QString("role %1 quantity").arg(roleKey),
                ok,
                QString::number(expected),
                QString::number(got)
                );
        }

        // 6) Többlet anyagok, amelyek nem szerepelnek a BOM-ban
        for (auto it3 = actual.begin(); it3 != actual.end(); ++it3)
        {
            QString rawKey = it3.key();
            QString roleKey = MaterialRoleUtils::normalizePrefix(rawKey);

            double got = it3.value();

            bool ok = bomRoleMap.contains(roleKey);

            result.entries.add(
                ref,
                "BOM",
                QString("role %1 exists in BOM").arg(roleKey),
                ok,
                "role present",
                ok ? "present" : QString("excess: %1").arg(got)
                );
        }

        // 7) Summary
        QString customer = list.first().ownerName.trimmed();
        if (customer.isEmpty())
            customer = "<ismeretlen>";

        auto& s = result.summary.perCustomer[customer];

        // tételszám-szintű hibadetektálás
        bool refHasError = result.entries.hasError_ByRef(ref);

        if (refHasError) {
            s.addReference_Bad(ref);
        } else {
            s.addReference_Good(ref);
        }
    }

    return result;
}

QStringList ProductBomAuditService::BOM_audit()
{
    QStringList out;
    out << "=== PRODUCT BOM AUDIT ===";

    const auto all = CuttingPlanRequestRegistry::instance().readAll();
    ProductBomAuditResult result = ProductBomAuditService::run(all);

    // 🔹 1) Csoportosítás tételszám szerint
    QHash<QString, QVector<BomAuditEntry>> perRef;
    for (const auto& e : result.entries.readAll()) {
        perRef[e.ref].append(e);
    }

    // 🔹 2) Szekciók kiírása
    for (auto it = perRef.begin(); it != perRef.end(); ++it)
    {
        const QString ref = it.key();
        const auto& entries = it.value();

        Cutting::Plan::Request *req = CuttingPlanRequestRegistry::instance().getFirstRequest(ref);

        // Megrendelő neve (HEAD auditból jön)
        QString customer = req?req->ownerName:"<ismeretlen>";
        out << QString("=== [%1] – %2 ===").arg(ref, customer);

        // 🔹 3) Sorok ✔️ / ❌ ikonokkal
        for (const auto& e : entries)
        {
            QString icon = e.passed ? "✅" : "❌";

            QString line = QString("%1 (%2) %3 | expected: %4 | actual: %5")
                               .arg(icon)
                               .arg(e.category)
                               .arg(e.testName)
                               .arg(e.expected)
                               .arg(e.actual);

            if (!e.details.isEmpty())
                line += QString(" | %1").arg(e.details);

            out << line;
        }

        // 🔹 4) Mini összegzés a tételszámhoz
        bool refHasError = result.entries.hasError_ByRef(ref);
        QString finalIcon = refHasError ? "❌" : "✅";
        out << QString("=== [%1] RESULT: %2 ===").arg(ref, finalIcon);
        out << "";

    }

    // 🔹 5) Összesítés megrendelőnként
    out << "=== ÖSSZESÍTÉS MEGRENDELŐNKÉNT ===";

    for (auto it = result.summary.perCustomer.begin();
         it != result.summary.perCustomer.end(); ++it)
    {
        const QString& customer = it.key();
        const CountPerType& s = it.value();

        // ikon a teljes megrendelőre
        QString icon;
        if (s.total() > 0 && s.bad() == 0)      icon = "✅";
        else if (s.total() > 0 && s.good() == 0) icon = "❌";
        else                                     icon = "•";

        // minden tételszám listázása pipával/X-el
        QStringList refLines;
        for (const QString& ref : s.goodRefs())
            refLines << QString("✅ %1").arg(ref);
        for (const QString& ref : s.badRefs())
            refLines << QString("❌ %1").arg(ref);

        QString refsJoined = refLines.join(", ");

        // szöveg: ha mind jó, vagy mind hibás, ne írjunk felesleges 0-kat
        QString summaryText;
        if (s.total() > 0 && s.bad() == 0) {
            summaryText = QString("minden tételszám jó (%1)").arg(refsJoined);
        } else if (s.total() > 0 && s.good() == 0) {
            summaryText = QString("minden tételszám hibás (%1)").arg(refsJoined);
        } else {
            summaryText = QString("jó=%1, hibás=%2 | %3")
                              .arg(s.good())
                              .arg(s.bad())
                              .arg(refsJoined);
        }

        QString line = QString("   %1 %2 | %3")
                           .arg(icon)
                           .arg(customer)
                           .arg(summaryText);

        out << line;
    }


    out << "=== PRODUCT BOM AUDIT VÉGE ===";

    return out;
}


