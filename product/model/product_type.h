#pragma once
#include <QString>
#include <QUuid>

struct ProductType {
    QUuid id;
    QString code;   // gépi azonosító
    QString name;   // emberi címke
};

