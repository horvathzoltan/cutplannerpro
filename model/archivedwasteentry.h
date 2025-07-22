#ifndef ARCHIVEDWASTEENTRY_H
#define ARCHIVEDWASTEENTRY_H

#include <QString>
#include <QUuid>
#include <QDateTime>

class ArchivedWasteEntry
{
public:
    ArchivedWasteEntry() = default;

    QUuid materialId;
    int wasteLength_mm = 0;
    QString sourceDescription;
    QDateTime createdAt;
    QString group;
    QString originBarcode;
    QString note;
    QUuid cutPlanId;

    QString toCSVLine() const;
};

#endif // ARCHIVEDWASTEENTRY_H
