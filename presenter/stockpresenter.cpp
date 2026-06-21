#include "stockpresenter.h"
#include "../model/registries/leftoverstockregistry.h"
#include "../model/registries/stockregistry.h"
#include "common/eventlogger.h"

#include <materials/registry/material_registry.h>

StockPresenter::StockPresenter(MainWindow* view, QObject* parent)
    : QObject(parent),
    view(view)
{
}

void StockPresenter::findMaterial(const QUuid& materialId, int minLength, int maxLength)
{   
    MaterialMaster mat = *MaterialRegistry::instance().findById(materialId);

    zEventINFO(QString("🔎 MaterialFinder: %1 | min=%2 | max=%3")
                   .arg(mat.toReportLabel())
                   .arg(minLength)
                   .arg(maxLength));


    // 2️⃣ Leftover keresés
    const auto leftovers = LeftoverStockRegistry::instance().readAll();
    for (const auto& e : leftovers)
    {
        if (e.materialId == materialId &&
            e.availableLength_mm >= minLength &&
            e.availableLength_mm <= maxLength)
        {
            zEventINFO(QString("♻️ Leftover found: %1 [%2] | %3 mm | storage=%4")
                           .arg(mat.toReportLabel())
                           .arg(e.barcode)
                           .arg(e.availableLength_mm)
                           .arg(e.storageName()));

            emit highlightLeftover(e.entryId);
            return;
        }
    }

    // 3️⃣ Stock fallback
    const auto stock = StockRegistry::instance().readAll();
    for (const auto& e : stock)
    {
        if (e.materialId == materialId && e.quantity > 0)
        {
            zEventINFO(QString("📦 Stock found: %1 | qty=%2 | storage=%3")
                           .arg(mat.toReportLabel())
                           .arg(e.quantity)
                           .arg(e.storageName()));


            emit highlightStock(e.entryId);
            return;
        }
    }

    // 4️⃣ Nincs leftover és nincs stock
    zEventINFO(QString("❌ No leftover or stock found: %1 | min=%2 | max=%3")
                   .arg(mat.toReportLabel())
                   .arg(minLength)
                   .arg(maxLength));

    emit showNotFoundMessage("Nincs megfelelő leftover vagy stock.");
}



// void StockPresenter::materialChosen(const StockEntry& entry)
// {
//     // 1) loggolás
//     zEventINFO(QString("📦 Material selected: %1 | %2 | qty=%3")
//                    .arg(entry.materialName())
//                    .arg(entry.storageName())
//                    .arg(entry.quantity));

//     // 2) minLength később dialogból jön
//     int minLength = 2000;
//     int maxLength = 3000;

//     // 3) keresés
//     findMaterial(entry.materialId, minLength, maxLength);
// }
