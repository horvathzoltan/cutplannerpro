#pragma once

#include "model/leftoverstockentry.h"
#include <model/cutting/plan/cutplan.h>

namespace Cutting {
namespace Optimizer {

namespace LineageHelper{
/*
 *     static void validateLineage(const Cutting::Plan::CutPlan &plan, const QVector<Cutting::Plan::CutPlan>& result_plans);
    static void validateLineage(const LeftoverStockEntry &entry, const QVector<Cutting::Plan::CutPlan>& result_plans);
    static QString lineageTree(const Cutting::Plan::CutPlan &plan, const QVector<Cutting::Plan::CutPlan>& result_plans);
    static QString lineageTree(const LeftoverStockEntry &entry, const QVector<Cutting::Plan::CutPlan>& result_plans);
 */


inline void validateLineage(
    const Cutting::Plan::CutPlan& plan, const QVector<Cutting::Plan::CutPlan>& result_plans)
{
    // 1) Gyökér: nincs parent → stock rúd
    if (!plan.parent().has_value()) {
        zInfo(QString("🔎 Lineage OK — ROOT (stock), barcode=%1")
                  .arg(plan.sourceBarcode));
        return;
    }

    const auto& parent = plan.parent().value();

    // 2) Barcode ellenőrzés
    if (parent.barcode().trimmed().isEmpty()) {
        zWarning(QString("⚠️ Lineage WARNING — empty parent barcode (planId=%1)")
                     .arg(plan.planId.toString()));
    }

    // 3) Ha nincs parent planId → stock gyökér
    if (!parent.planId().has_value()) {
        zInfo(QString("🔎 Lineage OK — parent is STOCK (barcode=%1)")
                  .arg(parent.barcode()));
        return;
    }

    // 4) Parent planId visszakeresése
    auto it = std::find_if(
        result_plans.begin(), result_plans.end(),
        [&](const Cutting::Plan::CutPlan& p){
            return p.planId == parent.planId().value();
        });

    if (it == result_plans.end()) {
        zWarning(QString("⚠️ Lineage WARNING — parent planId=%1 not found")
                     .arg(parent.planId()->toString()));
        return;
    }

    // 5) Ciklusdetektálás
    if (parent.planId().value() == plan.planId) {
        zError(QString("❌ Lineage ERROR — plan references itself! planId=%1")
                   .arg(plan.planId.toString()));
        return;
    }

    zInfo(QString("🔎 Lineage OK — parent=%1").arg(parent.toString()));
}

inline void validateLineage(
    const LeftoverStockEntry& entry, const QVector<Cutting::Plan::CutPlan>& result_plans)
{
    if (!entry._parent.has_value()) {
        zInfo("🔎 Lineage OK — leftover has no parent");
        return;
    }

    const auto& parent = entry._parent.value();

    if (parent.barcode().trimmed().isEmpty()) {
        zWarning(QString("⚠️ Lineage WARNING — leftover parent barcode empty (entryId=%1)")
                     .arg(entry.entryId.toString()));
    }

    if (parent.planId().has_value() && parent.planId()->isNull()) {
        zWarning(QString("⚠️ Lineage WARNING — leftover parent planId NULL (entryId=%1)")
                     .arg(entry.entryId.toString()));
    }

    zInfo(QString("🔎 Lineage OK — leftover parent=%1").arg(parent.toString()));
}

inline QString lineageTree(
    const Cutting::Plan::CutPlan& plan, const QVector<Cutting::Plan::CutPlan>& result_plans)
{
    QString out;
    out += "📐 LINEAGE TREE\n";
    out += QString("PLAN %1\n").arg(plan.planId.toString());

    // 1) Ha nincs parent → stock gyökér
    if (!plan.parent().has_value()) {
        out += QString(" └─ STOCK ROOT (barcode=%1)\n")
                   .arg(plan.sourceBarcode);
        return out;
    }

    const Cutting::Plan::ParentInfo* current = &plan.parent().value();
    QString prefix = " └─ ";

    while (current) {
        // 2) Node kiírása
        if (!current->planId().has_value()) {
            out += QString("%1STOCK (barcode=%2)\n")
            .arg(prefix)
                .arg(current->barcode());
            break;
        }

        out += QString("%1%2\n").arg(prefix, current->toString());

        // 3) Parent plan visszakeresése
        auto it = std::find_if(
            result_plans.begin(), result_plans.end(),
            [&](const Cutting::Plan::CutPlan& p){
                return p.planId == current->planId().value();
            });

        if (it == result_plans.end())
            break;

        if (!it->parent().has_value())
            break;

        current = &it->parent().value();
        prefix = "     " + prefix;
    }

    return out;
}

inline QString lineageTree(
    const LeftoverStockEntry& entry, const QVector<Cutting::Plan::CutPlan>& result_plans)
{
    QString out;
    out += "📐 LINEAGE TREE (Leftover)\n";
    out += QString("LEFTOVER %1 (%2 mm)\n")
               .arg(entry.barcode)
               .arg(entry.availableLength_mm);

    if (!entry._parent.has_value()) {
        out += " └─ no parent\n";
        return out;
    }

    const auto* current = &entry._parent.value();
    QString prefix = " └─ ";

    while (current) {
        out += QString("%1%2\n").arg(prefix, current->toString());

        if (!current->planId().has_value())
            break;

        auto it = std::find_if(
            result_plans.begin(), result_plans.end(),
            [&](const Cutting::Plan::CutPlan& p){
                return p.planId == current->planId().value();
            });

        if (it == result_plans.end())
            break;

        if (!it->parent().has_value())
            break;

        current = &it->parent().value();
        prefix = "     " + prefix;
    }

    return out;
}

}

} // end namespace Optimizer
} // end namespace Cutting