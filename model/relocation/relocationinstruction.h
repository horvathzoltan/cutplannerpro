#pragma once

#include <QString>


struct RelocationInstruction {
    QString materialCode;
    QString sourceLocation;
    QString targetLocation;
    int quantity;
};
