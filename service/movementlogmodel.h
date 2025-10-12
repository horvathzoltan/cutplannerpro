#pragma once

#include "common/movementdatahelper.h"
#include "model/movementdata.h"
#include "../common/logmeta.h"
#include "common/../common/csvhelper.h"
#include <QCoreApplication>
#include <QSysInfo>
//#include <QHostInfo>

struct MovementLogModel {
    LogMeta meta;
    MovementData movement;

    MovementLogModel() {
        meta.timestamp = QDateTime::currentDateTime();
        meta.appVersion = qApp->applicationVersion();
        meta.user = qgetenv("USER");
        meta.host = QSysInfo::machineHostName();//QHostInfo::localHostName();
    }

    explicit MovementLogModel(MovementData m) : MovementLogModel() {
        movement = std::move(m);
    }

    QString toCsvString() const {
        auto movement_log = MovementDataHelper::fromMovementData(movement);
        return QString("%1;%2;%3;%4;%5;%6;%7;%8;%9")
        .arg(meta.toString())
            .arg(CsvHelper::escape(movement_log.itemName))
            .arg(movement_log.itemBarcode)
            .arg(movement.quantity)
            // pl. dátum, user, host
            .arg(CsvHelper::escape(movement_log.fromStorageName))
            .arg(movement_log.fromBarcode)

            .arg(CsvHelper::escape(movement_log.toStorageName))
            .arg(movement_log.toBarcode)

            .arg(CsvHelper::escape(movement.comment));
    }

};
