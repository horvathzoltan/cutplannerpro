#pragma once

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
        return QString("%1;%2;%3;%4;%5;%6;%7;%8;%9")
        .arg(meta.toString())
            .arg(CsvHelper::escape(movement.itemName))
            .arg(movement.itemBarcode)
            .arg(movement.quantity)
            // pl. dátum, user, host
            .arg(CsvHelper::escape(movement.fromStorageName))
            .arg(movement.fromBarcode)

            .arg(CsvHelper::escape(movement.toStorageName))
            .arg(movement.toBarcode)

            .arg(CsvHelper::escape(movement.comment));
    }

};
