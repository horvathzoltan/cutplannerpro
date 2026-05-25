#pragma once

#include <QUuid>
#include <QString>

namespace Cutting {
namespace Plan {

struct ParentInfo{
    //annak a rúdnak a fizikai azonosítója, amelyből ez a CutPlan készült, ha ez egy leftover-ből készült új CutPlan, akkor a leftover forráskódja (RST-xxx), ha stockból készült, akkor a stock mesterséges kódja (MAT-xxx)
    QString barcode;
    //annak a CutPlan-nek az azonosítója, amely létrehozta a leftover-t, amiből ez a CutPlan készült
    std::optional<QUuid> planId;

    QString toString() const {
        QString planText = planId.has_value()
        ? planId->toString()
        : "—";
        return QString("%1 (plan=%2)").arg(barcode, planText);
    }
};

} //namespace Plan
}  //namespace Cutting