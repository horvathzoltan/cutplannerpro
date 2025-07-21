#include "cutplan.h"
#include "registries/materialregistry.h"
#include "../common/grouputils.h"

bool CutPlan::usedReusable() const
{
        return source == CutPlanSource::Reusable;
}


bool CutPlan::isFinalized() const
{
    // Completed vagy manuálisan lezárt (Abandoned) tervek már nem módosíthatók
    return status == CutPlanStatus::Completed || status == CutPlanStatus::Abandoned;
}

QString CutPlan::name() const
{
    // MaterialRegistry-ből lekérjük az anyag nevét
    auto opt = MaterialRegistry::instance().findById(materialId);
    return opt ? opt->name : "(ismeretlen)";
}

QString CutPlan::groupName() const
{
    // Csoportnév az anyag ID alapján — helperből
    return GroupUtils::groupName(materialId);
}

CutPlanStatus CutPlan::getStatus() const
{
    return status;
}

void CutPlan::setStatus(CutPlanStatus newStatus)
{
    status = newStatus;
}
