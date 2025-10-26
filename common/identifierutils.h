#pragma once
#include <QString>
#include <QUuid>

class IdentifierUtils {
public:
    static QString makeRodId(int rodNumber) {
        return QString("ROD-%1").arg(rodNumber, 4, 10, QChar('0'));
    }

    static QString makePieceId(const QUuid& pieceUuid) {
        return QString("PCS-%1").arg(pieceUuid.toString(QUuid::WithoutBraces).left(6));
    }

    static QString makeLeftoverId(const QUuid& leftoverUuid) {
        return QString("RST-%1").arg(leftoverUuid.toString(QUuid::WithoutBraces).left(6));
    }

    static QString makeSegmentId(const QUuid& segUuid) {
        return QString("SEG-%1").arg(segUuid.toString(QUuid::WithoutBraces).left(6));
    }

    static QString unidentified() {
        return "UNIDENTIFIED";
    }
};
