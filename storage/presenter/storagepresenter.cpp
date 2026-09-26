#include "storagepresenter.h"

#include <QPdfWriter>
#include <QPainter>
#include <QDir>
#include <QDateTime>

#include "common/eventlogger.h"
#include "stock/utils/materialbarcodelistform_utils.h"
#include "stock/utils/stockintakeform_utils.h"
#include "stock/utils/stocklistform_utils.h"
#include "stock/utils/storageqrcodelistform_utils.h"
#include "storage/registry/storageregistry.h"
#include "common/qrcodepainter.h"          // saját QR generátor
#include "storage/utils/storage_label_utils.h"
#include "view/MainWindow.h"

#include <materials/registry/material_rolegroup_registry.h>
#include <materials/registry/material_storagegroupregistry.h>

StoragePresenter::StoragePresenter(MainWindow* view, QObject* parent)
    : QObject(parent), _view(view)
{
}

void StoragePresenter::exportStorageLabelPdf(const QUuid& storageId)
{
    const StorageEntry* st = StorageRegistry::instance().findById(storageId);
    if (!st) {
        zEvent("❌ Ismeretlen tárhely ID.");
        return;
    }

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StorageLabel_%2_%3.pdf")
                       .arg(dir)
                       .arg(st->barcode)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    StorageLabelUtils::drawStorageLabel(painter, writer, pageRect, st, true);

    painter.end();
    zEvent(QString("📄 Tárhely címke exportálva: %1").arg(path));
}

void StoragePresenter::exportMultipleLabels(const QList<StorageEntry*>& entries)
{
    if (entries.isEmpty()) {
        zEvent("⚠️ Nincs kijelölt tárhely a címkékhez.");
        return;
    }

    // zInfo("=== Tömeges címke export indul ===");
    // for (const StorageEntry* st : entries) {
    //     zInfo(QString("➡️ %1 | %2 | %3")
    //               .arg(st->id.toString())
    //               .arg(st->name)
    //               .arg(st->barcode));
    // }
    // zInfo("=== Lista vége ===");

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StorageLabels_%2.pdf")
                       .arg(dir)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    // ⭐ 5 címke egy lapon
    const int maxPerPage = 5;
    qreal cellHeight = pageRect.height() / maxPerPage;

    int count = 0;

    for (const StorageEntry* st : entries) {

        if (count > 0 && count % maxPerPage == 0) {
            writer.newPage();   // új lap minden 5 címke után
        }

        int indexOnPage = count % maxPerPage;

        QRectF cellRect(
            pageRect.left(),
            pageRect.top() + indexOnPage * cellHeight,
            pageRect.width(),
            cellHeight
            );

        StorageLabelUtils::drawStorageLabel(painter, writer, cellRect, st, false);

        count++;
    }

    painter.end();
    zEvent(QString("📄 Tömeges tárhely címkék exportálva: %1").arg(path));
}

void StoragePresenter::exportStockIntakeForm(const QUuid& storageId)
{
    const StorageEntry* st = StorageRegistry::instance().findById(storageId);
    if (!st) {
        zEvent("❌ Ismeretlen tárhely ID.");
        return;
    }

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StockIntake_%2_%3.pdf")
                       .arg(dir)
                       .arg(st->barcode)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    painter.setFont(QFont("Noto Sans Mono", 11));

    // === 1) Tárhely címke a lap tetején ===
    QRectF labelRect(
        pageRect.left(),
        pageRect.top(),
        pageRect.width(),
        pageRect.height() * 0.25
        );

    painter.save();   // 🔥 painter állapot mentése

    StorageLabelUtils::drawStorageLabel(painter, writer, labelRect, st, false);

    painter.restore(); // 🔥 painter visszaállítása → megszűnik a skálázás

    // === 2) Anyagfelvételi táblázat leftover-stílusban ===
    QRectF tableRect(
        pageRect.left(),
        pageRect.top() + pageRect.height() * 0.25,
        pageRect.width(),
        pageRect.height() * 0.75
        );

    StockIntakeFormUtils::drawStockIntakeTable(painter, tableRect);

    painter.end();
    zEvent(QString("📄 Anyagfelvételi űrlap exportálva: %1").arg(path));
}

void StoragePresenter::exportStockListPdf(const QUuid& storageId)
{
    const StorageEntry* st = StorageRegistry::instance().findById(storageId);
    if (!st) {
        zEvent("❌ Ismeretlen tárhely ID.");
        return;
    }

    // készlet lekérése
    QList<StockEntry> entries =
        StockRegistry::instance().findByStorageId(storageId);

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StockList_%2_%3.pdf")
                       .arg(dir)
                       .arg(st->barcode)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    painter.setFont(QFont("Noto Sans Mono", 11));

    // 1) Tárhely címke
    painter.save();
    QRectF labelRect(
        pageRect.left(),
        pageRect.top(),
        pageRect.width(),
        pageRect.height() * 0.25
        );
    StorageLabelUtils::drawStorageLabel(painter, writer, labelRect, st, false);
    painter.restore();

    // 2) Készletlista táblázat
    QRectF tableRect(
        pageRect.left(),
        pageRect.top() + pageRect.height() * 0.25,
        pageRect.width(),
        pageRect.height() * 0.75
        );

    StockListFormUtils::drawStockListTable(painter, tableRect, entries);

    painter.end();
    zEvent(QString("📄 Készletlista exportálva: %1").arg(path));
}

void StoragePresenter::exportMaterialBarcodeList()
{
    // 1️⃣ Anyagok összegyűjtése
    QList<QUuid> materialIds;
    for (const auto& m : MaterialRegistry::instance().readAll())
        materialIds.append(m.id);

    if (materialIds.isEmpty()) {
        zEvent("ℹ️ Nincs anyag a törzsben.");
        return;
    }

    // 2️⃣ PDF létrehozása
    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/MaterialBarcodeList_%2.pdf")
                       .arg(dir)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    painter.setFont(QFont("Noto Sans Mono", 11));

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    // 3️⃣ Oldaltöréshez szükséges változók
    qreal y = pageRect.top() + 40.0;
    const qreal topMargin = 40.0;

    QFontMetrics fm(painter.font());

    const qreal textH = fm.height();      // pl. 14–18 px

    auto drawHeader = [&]() {
        QString title = QString("🏷️ Anyag Vonalkódjegyzék – %1")
                            .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm"));

        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         title);
        y += textH+40;

        // táblázat fejléc
        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         "External        Material + Barcode");
        y += textH + 40;

        painter.drawLine(pageRect.left() + 40, y,
                         pageRect.right() - 40, y);
        y += 10;
    };

    drawHeader();

    // 4️⃣ Sorok rajzolása több oldalon
    const qreal bcH   = 120;               // barcode magasság
    const qreal gap1  = 10;                // text → barcode gap
    const qreal gap2  = 20;               // barcode → következő sor gap

    const qreal rowHeight = textH + gap1 + bcH + gap2;

    for (const QUuid& id : materialIds) {

        if (y + rowHeight > pageRect.bottom() - 40) {
            writer.newPage();
            y = pageRect.top() + topMargin;
            drawHeader();
        }

        y = MaterialBarcodeListFormUtils::drawOneMaterialRow(
            painter,
            pageRect,
            y,
            id
            );
    }


    painter.end();
    zEvent(QString("📄 Anyag vonalkódjegyzék exportálva: %1").arg(path));
}


void StoragePresenter::exportStorageBarcodeList()
{
    // 1️⃣ Tárhelyek összegyűjtése
    QList<QUuid> storageIds;
    for (const auto& s : StorageRegistry::instance().readAll())
        storageIds.append(s.id);

    if (storageIds.isEmpty()) {
        zEvent("ℹ️ Nincs tárhely a rendszerben.");
        return;
    }

    // 2️⃣ PDF létrehozása
    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StorageQrcodeList_%2.pdf")
                       .arg(dir)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    painter.setFont(QFont("Noto Sans Mono", 11));

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    // 3️⃣ Oldaltöréshez szükséges változók
    qreal y = pageRect.top() + 40.0;
    const qreal topMargin = 40.0;

    QFontMetrics fm(painter.font());
    const qreal textH = fm.height();

    auto drawHeader = [&]() {
        QString title = QString("📦 Tárhely QR‑kód lista – %1")
                            .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm"));

        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         title);
        y += textH + 40;

        painter.drawText(QRectF(pageRect.left() + 40, y, pageRect.width(), textH),
                         Qt::AlignLeft,
                         "Storage Name        Logistic Barcode + QR");
        y += textH + 40;

        painter.drawLine(pageRect.left() + 40, y,
                         pageRect.right() - 40, y);
        y += 10;
    };

    drawHeader();

    // 4️⃣ Sorok rajzolása több oldalon
    const qreal qrH   = 150;   // QR magasság
    const qreal gap1  = 10;    // text → QR gap
    const qreal gap2  = 10;    // QR → következő sor gap

    const qreal rowHeight = textH + gap1 + qrH + gap2;
    int l = 0;
    for (const QUuid& id : storageIds) {

        if (y + rowHeight > pageRect.bottom() - 40) {
            writer.newPage();
            y = pageRect.top() + topMargin;
            drawHeader();
            l=1;
        }

        l++;
        y = StorageQrcodeListFormUtils::drawOneStorageRow(
            painter,
            pageRect,
            y,
            id, l
            );
    }

    painter.end();
    zEvent(QString("📄 Tárhely QR‑kód lista exportálva: %1").arg(path));
}

// QSet<QUuid> StoragePresenter::findCommonMaterials(const QList<StockListFormUtils::AggregatedMaterial>& mats)
// {
//     QMap<QUuid, QSet<QUuid>> matToSubtypes;

//     for (const auto& am : mats)
//     {
//         const MaterialMaster* m = MaterialRegistry::instance().findById(am.materialId);
//         if (!m) continue;

//         QString prefix = m->barcode.split('-').first();
//         MaterialRole role = MaterialRoleRegistry::instance().roleForBarcode(prefix);

//         matToSubtypes[am.materialId].insert(role.productSubtypeId);
//     }

//     QSet<QUuid> common;

//     for (auto it = matToSubtypes.begin(); it != matToSubtypes.end(); ++it)
//         if (it.value().size() > 1)
//             common.insert(it.key());

//     return common;
// }
QList<StockListFormUtils::AggregatedMaterial>
StoragePresenter::buildGroupedList(const QList<StockListFormUtils::AggregatedMaterial>& mats)
{
    auto makeSectionKey = [&](const QUuid& typeId,
                              const QUuid& subtypeId,
                              MaterialFamily fam,
                              bool isCommon)
    {
        auto *type = ProductTypeRegistry::instance().findById(typeId);
        QString typeName = type?type->name:"?";
        QString famName = MaterialFamilyUtils::toString(fam);

        if (isCommon)
            return QString("%1::COMMON::%2")
                .arg(typeName)
                .arg(famName);

        auto *subType = ProductSubtypeRegistry::instance().findById(subtypeId);
        QString subtypeName = subType?subType->name:"?";
        return QString("%1::%2::%3")
            .arg(typeName)
            .arg(subtypeName)
            .arg(famName);
    };

    QList<StockListFormUtils::AggregatedMaterial> result;
    QSet<QUuid> emitted;   // deduplikáció

    // --- 1) materialId → storageGroup
    QMap<QUuid, const MaterialStorageGroup*> matToStorageGroup;
    for (const auto& sg : MaterialStorageGroupRegistry::instance().readAll())
        for (const QUuid& matId : sg.members)
            matToStorageGroup[matId] = &sg;

    // --- 2) materialId → szerepkörök
    const auto allRoles = MaterialRoleRegistry::instance().readAll();
    QMap<QUuid, QVector<MaterialRole>> matToRoles;

    for (const auto& r : allRoles)
    {
        const MaterialStorageGroup* sg =
            MaterialStorageGroupRegistry::instance().findById(r.storageGroupId);

        if (!sg)
            continue;

        for (const QUuid& matId : sg->members)
            matToRoles[matId].append(r);
    }

    // --- 3) BOM sorrend
    const auto bom = BomRegistry::instance().readAll();
    QMap<QUuid, QMap<QUuid, QList<MaterialFamily>>> bomOrder;

    for (const auto& e : bom)
        bomOrder[e.productTypeId][e.productSubtypeId].append(e.family);

    // --- 4) Csoportok
    QList<StockListFormUtils::AggregatedMaterial> ungrouped;
    QMap<QUuid, QMap<MaterialFamily, QList<StockListFormUtils::AggregatedMaterial>>> commonByTypeFamily;
    QMap<QUuid, QMap<QUuid, QMap<MaterialFamily, QList<StockListFormUtils::AggregatedMaterial>>>> specificByTypeSubtypeFamily;

    for (const auto& am : mats)
    {
        const QUuid matId = am.materialId;

        if (!matToStorageGroup.contains(matId)) {
            ungrouped.append(am);
            continue;
        }

        const auto roles = matToRoles.value(matId);

        if (roles.isEmpty()) {
            ungrouped.append(am);
            continue;
        }

        const MaterialMaster* mm = MaterialRegistry::instance().findById(matId);
        if (!mm)
            continue;

        if (roles.size() > 1) {
            QUuid typeId = roles.first().productTypeId;
            MaterialFamily fam = mm->family;
            commonByTypeFamily[typeId][fam].append(am);
        } else {
            const auto& r = roles.first();
            specificByTypeSubtypeFamily[r.productTypeId][r.productSubtypeId][mm->family].append(am);
        }
    }

    // --- 5) BOM sorrend szerinti kilistázás
    for (auto typeIt = bomOrder.begin(); typeIt != bomOrder.end(); ++typeIt)
    {
        QUuid typeId = typeIt.key();
        auto& subtypeMap = typeIt.value();

        for (auto subIt = subtypeMap.begin(); subIt != subtypeMap.end(); ++subIt)
        {
            QUuid subtypeId = subIt.key();
            const auto& familyOrder = subIt.value();

            for (MaterialFamily fam : familyOrder)
            {
                // közös
                for (const auto& am0: commonByTypeFamily[typeId][fam])
                    if (!emitted.contains(am0.materialId)) {
                        auto am = am0;
                        am.sectionKey = makeSectionKey(typeId, QUuid(), fam, true);
                        result.append(am);
                        emitted.insert(am.materialId);
                    }

                // specifikus
                for (const auto& am0 : specificByTypeSubtypeFamily[typeId][subtypeId][fam])
                    if (!emitted.contains(am0.materialId)) {
                        auto am = am0;
                        am.sectionKey = makeSectionKey(typeId, subtypeId, fam, false);
                        result.append(am);
                        emitted.insert(am.materialId);
                    }
            }
        }
    }

    // --- 6) BOM‑on kívüli anyagok (fallback)
    for (const auto& am0 : ungrouped)
        if (!emitted.contains(am0.materialId)) {
            auto am = am0;
            am.sectionKey = "Egyéb::SzerepkörNélküli";
            result.append(am);
            emitted.insert(am.materialId);
        }

    // --- 7) Globális fallback: minden anyag kerüljön be
    for (const auto& am0 : mats)
        if (!emitted.contains(am0.materialId)) {
            auto am = am0;
            am.sectionKey = "Egyéb::Maradék";
            result.append(am);
            emitted.insert(am.materialId);
        }

    return result;
}



void StoragePresenter::exportGlobalStockListPdf()
{
    QList<StockEntry> entries = StockRegistry::instance().readAll();

    QMap<QUuid, StockListFormUtils::AggregatedMaterial> map;

    auto virtualStorage =  StorageRegistry::instance().findByBarcode("VIRT");
    if(!virtualStorage) {
        zWarning("Nincs virtuális tárhely definiálva");
    }

    for (const auto& e : entries)
    {
        const MaterialMaster* master =
            MaterialRegistry::instance().findById(e.materialId);

        if (!master)
            continue;


        if(virtualStorage  && e.storageId == virtualStorage->id)
            continue;

        if (master->kind == MaterialKind::Simple)
        {
            auto& m = map[e.materialId];
            m.materialId = e.materialId;
            //m.master = master;
            m.totalQty += e.quantity;

            StockListFormUtils::AggregatedSte a =
                StockListFormUtils::buildAggregatedSte(e,1);

            m.storages.append(a);
            continue;
        }
        else if (master->kind == MaterialKind::Bundle)
        {
            const BundleDefinition* def =
                BundleRegistry::instance().findByCode(master->bundleCode);

            if (!def)
                continue;

            for (const auto& comp : def->components)
            {
                QUuid compId = comp.materialId;
                int compTotal = e.quantity * comp.count;

                // const MaterialMaster* cm =
                //     MaterialRegistry::instance().findById(compId);

                auto& m = map[compId];
                m.materialId = compId;
                //m.master = cm;
                m.totalQty += compTotal;

                // opcionális: tárolási helyek listája

                StockListFormUtils::AggregatedSte a =
                    StockListFormUtils::buildAggregatedSte(e,comp.count);

                m.storages.append(a);
            }

            continue;
        }
    }

     QList<StockListFormUtils::AggregatedMaterial> aggregatedMaterials =
         buildGroupedList(map.values());


   // QList<StockListFormUtils::AggregatedMaterial> aggregatedMaterials = map.values();

    QString dir = "_reports";
    QDir().mkpath(dir);

    QString path = QString("%1/StockList_GLOBAL_%2.pdf")
                       .arg(dir)
                       .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    painter.setFont(QFont("Noto Sans Mono", 11));

    QFontMetrics fm(painter.font());
    qreal lineH = fm.height();      // 🔥 VALÓDI sor-magasság

    auto drawHeader = [&](qreal& y){
        painter.drawText(QRectF(40, y, pageRect.width(), lineH),
                         Qt::AlignLeft,
                         "📦 Globális készletlista");
        y += lineH;

        painter.drawText(QRectF(40, y, pageRect.width(), lineH),
                         Qt::AlignLeft,
                         QString("📅 Dátum: %1")
                             .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm")));
        y += lineH;
        y += lineH * 0.5;
    };

    qreal y = pageRect.top();
    drawHeader(y);

    QString lastSectionKey;

    for (const auto& m : aggregatedMaterials)
    {
        if (m.sectionKey != lastSectionKey)
        {
            // új szekció kezdődik
            lastSectionKey = m.sectionKey;

            // oldaltörés ha nem férne ki
            qreal needed = lineH * 1.4;
            if (y + needed > pageRect.bottom()) {
                writer.newPage();
                painter.begin(&writer);
                painter.setFont(QFont("Noto Sans Mono", 11));
                y = pageRect.top();
                drawHeader(y);
            }

            painter.setFont(QFont("Noto Sans Mono", 12, QFont::Bold));
            painter.drawText(QRectF(40, y, pageRect.width(), needed),
                             Qt::AlignLeft,
                             QString("=== %1 ===").arg(m.sectionKey));
            y += needed;

            painter.setFont(QFont("Noto Sans Mono", 11));
        }
        int storageIndex = 0;

        while (storageIndex < m.storages.size())
        {
            auto res = drawSingleStockRow(painter, pageRect, y, m, storageIndex);

            if (res.pageBreakNeeded)
            {
                writer.newPage();
                painter.begin(&writer);
                painter.setFont(QFont("Noto Sans Mono", 11));

                y = pageRect.top();
                drawHeader(y);

                storageIndex = res.nextStorageIndex;   // folytatás innen
            }
            else
            {
                y = res.newY;
                storageIndex = res.nextStorageIndex;   // mehet tovább
            }
        }
    }


    painter.end();
    zEvent(QString("📄 Globális készletlista exportálva: %1").arg(path));
}


