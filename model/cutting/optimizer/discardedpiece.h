#pragma once

#include <QUuid>


// --- DiscardedPiece: géphez kötött eldobott darabok tárolása ---
struct DiscardedPiece {
    QUuid pieceId;
    QUuid requestId;
    QUuid materialId;
    QUuid machineId;
    QString failReason;
};