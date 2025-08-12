#pragma once

#include <QUuid>


struct MovementData {
    QUuid fromEntryId;
    QUuid toStorageId;
    int quantity;
    QString comment;
};
