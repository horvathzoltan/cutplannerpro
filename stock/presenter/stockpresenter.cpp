#include "stockpresenter.h"
#include "stock/registry/stockregistry.h"
#include "common/eventlogger.h"

#include <materials/registry/material_registry.h>

#include "leftover/registry//leftoverstockregistry.h"
#include "storage/registry/storageregistry.h"
#include "storage/utils/storageutils.h"
#include <view/MainWindow.h>

#include <storage/model/storagetype.h>

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

QSet<QUuid> StockPresenter::collectSubtreeStorageIds(const QUuid& rootId)
{
    QSet<QUuid> result;
    result.insert(rootId);

    const auto& storages = StorageRegistry::instance().readAll();

    bool added = true;
    while (added) {
        added = false;
        for (const auto& s : storages) {
            if (result.contains(s.parentId) && !result.contains(s.id)) {
                result.insert(s.id);
                added = true;
            }
        }
    }
    return result;
}

void StockPresenter::filterStockByStorage(const QUuid& storageId)
{
    QSet<QUuid> subtree = collectSubtreeStorageIds(storageId);
    view->getStockTableManager()->refresh_TableFiltered(subtree);
}


QString StockPresenter::ReportRunOutMaterials()
{
    const auto& stock = StockRegistry::instance().readAll();

    struct MatInfo {
        int total = 0;
        QList<QPair<const StorageEntry*, int>> locations;
        QString materialName;
    };

    QHash<QString, MatInfo> map;

    for (const auto& e : stock)
    {
        const StorageEntry* st = e.storage();
        if (!st) continue;

        // Virtuális tárhely kihagyása
        if (st->barcode == "VIRT")
            continue;

        QString matBarcode = e.materialBarcode();
        MatInfo& info = map[matBarcode];

        // anyag neve
        if (info.materialName.isEmpty()) {
            const MaterialMaster* mm = e.master();
            info.materialName = mm ? mm->toDisplay() : matBarcode;
        }

        // összes mennyiség
        info.total += e.quantity;

        // tárhely + mennyiség
        info.locations.append({ st, e.quantity });
    }

    QStringList veryLow;
    QStringList low;
    QStringList normal;

    veryLow << "🔴 Nagyon kevés (<10 db):";
    low     << "🟠 Kevés (<20 db):";
    normal  << "🟢 Teljes készlet (Virtuális nélkül):";

    for (auto it = map.begin(); it != map.end(); ++it)
    {
        const QString& matBarcode = it.key();
        const MatInfo& info = it.value();

        QString header = QString("%1  (%2)").arg(matBarcode, info.materialName);

        QStringList lines;
        lines << header;

        for (const auto& loc : info.locations)
        {
            const StorageEntry* st = loc.first;
            int qty = loc.second;

            QString logistic = StorageRegistry::instance().generateLogisticBarcode(st);

            lines << QString("   - %1  (%2) : %3 db")
                         .arg(st->barcode)
                         .arg(logistic)
                         .arg(qty);
        }

        lines << QString("   Összesen: %1 db").arg(info.total);
        lines << "";

        // kategorizálás
        if (info.total < 10)
            veryLow << lines.join("\n");
        else if (info.total < 20)
            low << lines.join("\n");
        else
            normal << lines.join("\n");

    }

    QString report;

    if (veryLow.size() > 1)
        report += veryLow.join("\n") + "\n";
    else
        report += "✔ Nincs nagyon kevés készlet (<10).\n\n";

    if (low.size() > 1)
        report += low.join("\n") + "\n";
    else
        report += "✔ Nincs kevés készlet (<20).\n\n";

    report += normal.join("\n");

    return report;
}


QString StockPresenter::ReportStorageAudit()
{
    const auto& storages = StorageRegistry::instance().readAll();
    const auto& stock = StockRegistry::instance().readAll();

    // storageId → list of stock entries
    QHash<QUuid, QList<const StockEntry*>> map;

    for (const auto& e : stock)
    {
        const StorageEntry* st = e.storage();
        if (!st) continue;

        if (st->barcode == "VIRT")
            continue;

        map[st->id].append(&e);
    }

    QStringList out;

    for (const auto& st : storages)
    {
        if (st.barcode == "VIRT")
            continue;

        const auto& entries = map.value(st.id);

        // ⭐ Meghatározzuk, hogy rakathely-e
        bool isRakathely = st.type.isLocation();

        // ⭐ Nem rakathely → csak akkor listázzuk, ha VAN készlet
        if (!isRakathely && entries.isEmpty())
            continue;

        QString logistic = StorageRegistry::instance().generateLogisticBarcode(&st);
        QString path = StorageRegistry::instance().uniqueHumanName(st.id);

        out << QString("📦 Storage: %1").arg(st.barcode);
        out << QString("🔎 Logisztikai kód: %1").arg(logistic);
        out << QString("📍 Path: %1").arg(path);

        // ⭐ Nem rakathely + van készlet → figyelmeztetés
        if (!isRakathely && !entries.isEmpty())
            out << "⚠️ NEM ENGEDÉLYEZETT LOKÁCIÓN LÉVŐ KÉSZLET!";

        out << "──────────────────────────────────";

        if (entries.isEmpty()) {
            // ⭐ Rakathely → üresen is listázzuk
            out << "   (nincs készlet)";
        } else {
            for (const auto* e : entries)
            {
                const MaterialMaster* mm = e->master();
                QString matName = mm ? mm->toDisplay() : "?";

                // ⭐ lastSeenAt kiírása
                QString lastSeen(L("last audit: %1").arg(e->lastSeenAt.toString("yyyy-MM-dd HH:mm")));

                out << QString("   %1 (%2) : %3 db : %4")
                           .arg(e->materialBarcode())
                           .arg(matName)
                           .arg(e->quantity)
                           .arg(lastSeen);
            }
        }

        out << "──────────────────────────────────";
        out << "";
    }

    return out.join("\n");
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
