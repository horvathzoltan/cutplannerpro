#include "product_bom_audit_service.h"

#include "common/logger.h"
#include "audit_header_rules.h"
#include "audit_length_rules.h"
#include "product/material_role_utils.h"
#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>
#include <materials/registry/material_registry.h>

#include <materials/model/material_family_utils.h>


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

        zInfo("Tételszám:"+ref + ", "
              + (!list.isEmpty()
                    ?"anyaglista: "+QString::number(list.count())+"db"
                    :"nincs anyag"));
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

        if (typeMismatch) {
            result.messages.append({
                ref,
                "❌ Típus/altípus keveredés egy tételszámon belül",
                true
            });
            continue;
        }

        // 2/a) FEJADATELLENŐRZÉS
        auto head = AuditHeaderRules::check(list);
        for (const QString& msg : head.messages)
            result.messages.append({ ref, msg, head.hasError });

        // if (!head.hasValidDimensions)
        //     continue;  // méretellenőrzés nem fut

        // 2/b) MÉRETELLENŐRZÉS
        auto len = AuditLengthRules::check(list, typeId, subtypeId);
        for (const QString& msg : len.messages)
            result.messages.append({ ref, msg, len.hasError });

        // 3) BOM lekérése (family → quantity)
        auto bomMap = BomRegistry::instance().bomMap(typeId, subtypeId);

        if (bomMap.isEmpty()) {
            result.messages.append({
                ref,
                "❌ Nincs BOM ehhez a termékhez",
                true
            });
            continue;
        }

        // 4) Request anyagok aggregálása role‑alapú family szerint
        QHash<QString, double> actual;

        for (const auto& r : list)
        {
            const auto* mm = MaterialRegistry::instance().findById(r.materialId);
            if (!mm) {
                result.messages.append({
                    ref,
                    "❌ Ismeretlen anyag ID: " + r.materialId.toString(),
                    true
                });
                continue;
            }

            // NORMALIZÁLT ROLE
                    MaterialRole normalized =
                MaterialRoleUtils::makeRole(r, mm);

            if (normalized.family == MaterialFamily::Unknown ||
                normalized.barcodePrefix.isEmpty())
            {
                result.messages.append({
                    ref,
                    "❌ Ismeretlen role a barcode-hoz: " + mm->barcode,
                    true
                });
                continue;
            }

            QString msg = L("barcode: %1 -> family: %2 , role: %3")
                              .arg(mm->barcode)
                              .arg(MaterialFamilyUtils::toString(normalized.family))
                              .arg(normalized.barcodePrefix);
            zInfo(msg);

            QString roleKey = normalized.barcodePrefix.trimmed();
            actual[roleKey] += r.quantity;
        }



        auto bomRoleMap = BomRegistry::instance().bomRoleMap(typeId, subtypeId);

        bool hasError = false;

        int productCount = list.first().quantity;

        for (auto it2 = bomRoleMap.begin(); it2 != bomRoleMap.end(); ++it2)
        {
            QString rawKey = it2.key();
            // NORMALIZÁLT ROLE

            QString roleKey = MaterialRoleUtils::normalizePrefix(rawKey);


            double expectedPerProduct = it2.value(); // BOM: db / termék
            double expected = expectedPerProduct * productCount;

            double got = actual.value(roleKey, 0.0);

            if (got < expected) {
                hasError = true;
                result.messages.append({
                    ref,
                    QString("❌ HIÁNY %1 (%2) < elvárt (%3)")
                        .arg(roleKey)
                        .arg(got)
                        .arg(expected),
                    true
                });
            }
            else if (got > expected) {
                hasError = true;
                result.messages.append({
                    ref,
                    QString("❌ TÖBBLET %1 (%2) > elvárt (%3)")
                        .arg(roleKey)
                        .arg(got)
                        .arg(expected),
                    true
                });
            }
        }

        // 6) Többlet anyagok, amelyek nem szerepelnek a BOM-ban
        for (auto it3 = actual.begin(); it3 != actual.end(); ++it3)
        {
            QString rawKey = it3.key();
            QString roleKey = MaterialRoleUtils::normalizePrefix(rawKey);

            double got = it3.value();

            if (!bomRoleMap.contains(roleKey)) {
                hasError = true;
                result.messages.append({
                    ref,
                    QString("❌ TÖBBLET: %1 (%2 felesleg)")
                        .arg(roleKey)
                        .arg(got),
                    true
                });
            }
        }

        // 8) Összesítés megrendelőnként
        QString customer = list.first().ownerName.trimmed();
        if (customer.isEmpty())
            customer = "<ismeretlen>";

        auto& s = result.summary.perCustomer[customer];
        s.total++;

        if (hasError) {
            s.bad++;
            s.badRefs << ref;
        } else {
            s.good++;
        }
    }

    return result;
}

