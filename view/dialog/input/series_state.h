#pragma once

#include <QString>
#include <QVector>
#include <QSet>
#include <QPair>
#include <QUuid>

/**
 * ActiveSeries = workflow‑iránytű
 *
 * Nem tartalmazza a teljes sorozat adatait.
 * Nem tartalmaz Request objektumokat.
 *
 * Csak azt jelzi:
 *  - hol tartunk a sorozatban,
 *  - milyen tételszámokat jártunk be,
 *  - milyen BOM‑anyagok vannak soron,
 *  - mely cellák lettek már felvéve,
 *  - melyik cella az aktuális.
 *
 * A SeriesMatrixView a Presenter‑ből kapja a teljes sorozatot,
 * és ezt az ActiveSeries‑szel kombinálja.
 */
struct ActiveSeries
{
    bool active = false;

    // A sorozat első tételszáma
    QString startRef;

    // A sorozat tételszámai a bejárás sorrendjében
    QVector<QString> order;

    // BOM anyagok sorrendje (materialId lista)
    QVector<QUuid> bomMaterials;

    // Aktuális BOM‑anyag indexe (sor index)
    int currentMaterialIndex = 0;

    // Aktuális tételszám indexe (oszlop index)
    int currentColumnIndex = 0;

    // Felvitt cellák: (tételszám, materialId)
    QSet<QPair<QString, QUuid>> filledCells;

    // Szerkesztési mód jelző
    bool editingMode = false;

    // Reset funkció (új sorozat indításakor)
    void reset()
    {
        active = false;
        startRef.clear();
        order.clear();
        bomMaterials.clear();
        currentMaterialIndex = 0;
        currentColumnIndex = 0;
        filledCells.clear();
    }
};
