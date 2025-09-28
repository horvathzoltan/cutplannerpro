#pragma once

#include <QString>


struct RelocationInstruction {
    QString materialCode;
    QString sourceLocation;
    QString targetLocation;
    int quantity;
    bool isSatisfied = false; // új mező: jelzi, hogy ez nem mozgatás, hanem megerősítés
    QString barcode; // új mező: azonosító, különösen hullóknál
};
