#pragma once

#include <QUuid>


struct PowderConsumptionModel
{
    QUuid productTypeId;
    QUuid productSubtypeId;

    // emberi norma: hány kg por kell 1 méter profilhoz
    // ehhez megadjuk, hány méterre kell számolni
    double length = 0.0;

    // hány kiló porral számolunk a fenti hossz festéséhez?
    double weight = 0.0;

    // opcionális: színfüggő korrekció
    double colorMultiplier = 1.0;

    // opcionális: felületkezelés függő korrekció
    double surfaceMultiplier = 1.0;

    inline double kgPerMeterCorrected() const
    {
        if (length <= 0.0)
            return 0.0;

        double base = weight / length;
        return base * colorMultiplier * surfaceMultiplier;
    }

};
