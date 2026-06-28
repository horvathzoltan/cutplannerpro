#include "product_bom_audit_service.h"

#include "common/logger.h"
#include "naphalo_audit_service.h"            // groupByExternalRef reuse
#include "naphalo_material_aggregator.h"
#include "naphalo_bom_rules.h"
#include "naphalo_type_detector.h"

#include <product/registry/bom_registry.h>
#include <product/registry/material_role_registry.h>

#include <materials/registry/material_registry.h>

#include <materials/model/material_family_utils.h>

ProductBomAuditResult ProductBomAuditService::run(const QVector<Cutting::Plan::Request>& all)
{
    ProductBomAuditResult result;

    // 1) Tételszámok csoportosítása
    auto groups = NaphaloAuditService::groupByExternalRef(all);

    for (auto it = groups.begin(); it != groups.end(); ++it)
    {
        const QString ref = it.key();
        const auto& list = it.value();

        if (list.isEmpty())
            continue;

        // 2) Típus/altípus konzisztencia ellenőrzése
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

        // 4) Request anyagok aggregálása family szerint
        QHash<MaterialFamily, double> actual;

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

            MaterialFamily fam =
                MaterialRoleRegistry::instance().familyForBarcode(mm->barcode);

            QString msg = L("barcode: %1 -> family: %2")
                              .arg(mm->barcode)
                              .arg(MaterialFamilyUtils::toString(fam));
            zInfo(msg);

            actual[fam] += r.quantity;
        }

        // 5) BOM vs actual összevetése
        bool hasError = false;

        for (auto it2 = bomMap.begin(); it2 != bomMap.end(); ++it2)
        {
            MaterialFamily fam = it2.key();
            double expected = it2.value();
            double got = actual.value(fam, 0.0);

            if (got < expected) {
                hasError = true;
                result.messages.append({
                    ref,
                    QString("❌ HIÁNY %1 (%2) < elvárt (%3)")
                        .arg(MaterialFamilyUtils::toString(fam))
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
                        .arg(MaterialFamilyUtils::toString(fam))
                        .arg(got)
                        .arg(expected),
                    true
                });
            }
        }

        // 6) Többlet anyagok, amelyek nem szerepelnek a BOM-ban
        for (auto it3 = actual.begin(); it3 != actual.end(); ++it3)
        {
            MaterialFamily fam = it3.key();
            double got = it3.value();

            if (!bomMap.contains(fam)) {
                hasError = true;
                result.messages.append({
                    ref,
                    QString("❌ TÖBBLET: %1 (%2 felesleg)")
                        .arg(MaterialFamilyUtils::toString(fam))
                        .arg(got),
                    true
                });
            }
        }

        // // 7) Kitting lista generálása
        // for (auto it4 = bomMap.begin(); it4 != bomMap.end(); ++it4)
        // {
        //     MaterialFamily fam = it4.key();
        //     double expected = it4.value();
        //     double got = actual.value(fam, 0.0);

        //     if (got < expected) {
        //         double missing = expected - got;
        //         result.messages.append({
        //             ref,
        //             QString("❌ HIÁNY: %1 (%2 hiányzik)")
        //                 .arg(MaterialFamilyUtils::toString(fam))
        //                 .arg(missing),
        //             true
        //         });
        //     }
        // }

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

