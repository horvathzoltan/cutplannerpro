#include "archivedwasteentry.h"

QString ArchivedWasteEntry::toCSVLine() const {
    return QString("%1,%2,\"%3\",%4,\"%5\",\"%6\",\"%7\",%8")
    .arg(materialId.toString())
        .arg(wasteLength_mm)
        .arg(sourceDescription)
        .arg(createdAt.toString(Qt::ISODate))
        .arg(group)
        .arg(originBarcode)
        .arg(note)
        .arg(cutPlanId.toString());
}
