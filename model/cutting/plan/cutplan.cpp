#include "cutplan.h"
#include <QDebug>
#include "../../registries/materialregistry.h"
#include "../../../common/grouputils.h"

namespace Cutting {
namespace Plan {

bool CutPlan::usedReusable() const
{
        return source == Cutting::Plan::Source::Reusable;
}

bool CutPlan::isFinalized() const
{
    // Completed vagy manuálisan lezárt (Abandoned) tervek már nem módosíthatók
    return status == Status::Completed || status == Status::Abandoned;
}

QString CutPlan::materialName() const
{
    // MaterialRegistry-ből lekérjük az anyag nevét
    auto opt = MaterialRegistry::instance().findById(materialId);
    return opt ? opt->name : "(?)";
}

QString CutPlan::materialGroupName() const
{
    // Csoportnév az anyag ID alapján — helperből
    return GroupUtils::groupName(materialId);
}

Status CutPlan::getStatus() const
{
    return status;
}

void CutPlan::setStatus(Status newStatus)
{
    status = newStatus;
}

QString CutPlan::pieceLengthsAsString() const {
    QStringList out;
    for (const Cutting::Segment::SegmentModel& s : segments) {
        if (s.type == Cutting::Segment::SegmentModel::Type::Piece)
            out << QString::number(s.length_mm);
    }
    return out.join(";");
}


} //endof namespace Plan
} //endof namespace Cutting

