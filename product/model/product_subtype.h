#pragma once
#include <QString>
#include <QUuid>

struct ProductSubtype {
    QUuid id;
    QUuid typeId;
    QString code;   // gépi azonosító
    QString name;   // emberi címke
};

