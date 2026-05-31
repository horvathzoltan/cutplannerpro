#pragma once
#include "common/logger.h"
#include <QString>
#include <QUuid>

class IdentifierUtils {
public:
    static QString makeRodId(int counter) {
        int base = 22; // ugyanannyi, mint a safeLetterPrefix allowed.size()

        QString prefix;
        int n = counter;
        while (n > 0) {
            n--; // 0-index korrekció
            prefix.prepend(safeLetterPrefix(n % base));
            n /= base;
        }

        auto a =  QString("Rod-%1").arg(prefix);
        return a;
    }

    // 🆔 Darab azonosító (UUID rövidítve)
    static QString makePieceId(const QUuid& pieceUuid) {
        return QString("PCS-%1").arg(pieceUuid.toString(QUuid::WithoutBraces).left(6));
    }

    // 🆔 Szegmens azonosító (UUID rövidítve)
    static QString makeSegmentId(const QUuid& segUuid) {
        return QString("SEG-%1").arg(segUuid.toString(QUuid::WithoutBraces).left(6));
    }

    static QString unidentified() {
        return "UNIDENTIFIED";
    }

    /**
     * @brief Biztonságos betűkészlet prefixhez
     * - Kihagyjuk az I, L, O, Q betűket (összetéveszthetők)
     */
    static QChar safeLetterPrefix(int index) {
        static const QString allowed = "ABCDEFGHJKMNPQRSTUVWXYZ";
        return allowed[index % allowed.size()];
    }

    /**
     * @brief Általános azonosító generátor
     * - prefix: "MAT" vagy "RST"
     * - 1–999: PREFIX-001
     * - 1000 felett: PREFIX-A-123, PREFIX-B-456 ...
     */
    static QString makeGenericId(const QString& prefix, int counter) {
        if (counter < 1000) {
            // 0–999 → numerikus
            return QString("%1-%2").arg(prefix).arg(counter, 3, 10, QChar('0'));
        }

        int high = counter / 1000;   // blokk index
        int low  = counter % 1000;   // maradék

        static const QString allowed = "ABCDEFGHJKMNPQRSTUVWXYZ";
        int base = allowed.size(); // 22

        QString letterPrefix;
        if (high <= base) {
            // Egybetűs prefix
            letterPrefix = QString(allowed[high - 1]);
        } else {
            // Kétbetűs prefix (pl. AA, AB, …)
            int idx = high - 1;
            int first = (idx - 1) / base;
            int second = (idx - 1) % base;
            letterPrefix = QString("%1%2").arg(allowed[first]).arg(allowed[second]);
        }

        return QString("%1-%2-%3")
            .arg(prefix)
            .arg(letterPrefix)
            .arg(low, 3, 10, QChar('0'));
    }

    /// Anyag (stock) azonosító
    static QString makeMaterialId(int counter) {
        return makeGenericId("MAT", counter);
    }

    /// Leftover (hulló) azonosító
    static QString makeLeftoverId(int counter) {
        return makeGenericId("RST", counter);
    }

    static QString makeManualLeftoverId(int counter) {
        return makeGenericId("RSM", counter);
    }

};
