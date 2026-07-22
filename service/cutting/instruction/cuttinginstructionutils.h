#pragma once
#include <QDateTime>
#include <QPainter>
#include <QPdfWriter>
#include <QSet>

#include "../../../model/cutting/instruction/cutinstruction.h"
#include "common/barcodepainter.h"
#include "common/emojihelper.h"
#include "common/texthelper.h"
#include "labelmodel.h"
#include <materials/model/material_master.h>
#include <materials/registry/material_registry.h>
#include <model/registries/cuttingmachineregistry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/stockregistry.h>
#include <model/cutting/cuttingmachine.h>
#include <model/storageaudit/storageauditrow.h>
#include <service/storageaudit/storageauditservice.h>
#include <common/identifierutils.h>
#include "product/subtype_utils.h"
#include "settings/settingsmanager.h"


namespace CuttingInstructionUtils {

enum class SortStrategy {
    BySizeDesc,
    ByMaterialThenSize,
    //ByStatus,
    None
};


inline void postProcessMachineCuts(MachineCuts& mc, SortStrategy strategy = SortStrategy::BySizeDesc) {

    // 1️⃣ Rendezés a stratégiának megfelelően
    switch (strategy) {
    case SortStrategy::BySizeDesc:
        // std::sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
        //           [](const CutInstruction& a, const CutInstruction& b){
        //               return a.cutSize_mm > b.cutSize_mm;
        //           });
        std::stable_sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
                         [](const CutInstruction& a, const CutInstruction& b){
                             if (a.cutSize_mm != b.cutSize_mm)
                                 return a.cutSize_mm > b.cutSize_mm;    // 1️⃣ méret szerint
                             return a.rodId < b.rodId;                  // 2️⃣ rúd szerint
                         });

        break;

    case SortStrategy::ByMaterialThenSize:
        std::stable_sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
                         [](const CutInstruction& a, const CutInstruction& b){
                             if (a.materialId == b.materialId)
                                 return a.cutSize_mm > b.cutSize_mm;
                             return a.materialId.toString() < b.materialId.toString();
                         });
        break;

        // case SortStrategy::ByStatus:
        //     std::stable_sort(mc.cutInstructions.begin(), mc.cutInstructions.end(),
        //                      [](const CutInstruction& a, const CutInstruction& b){
        //                          return static_cast<int>(a.status) < static_cast<int>(b.status);
        //                      });
        //     break;

    case SortStrategy::None:
        // nincs rendezés
        break;
    }

    // 2️⃣ Kompenzációs logika
    double comp = mc.machineHeader.stellerCompensation_mm.value_or(0.0);
    double maxLen = mc.machineHeader.stellerMaxLength_mm.value_or(-1);

    for (auto& ci : mc.cutInstructions) {
        ci.isManualCut = false;
        ci.effectiveCutSize_mm = ci.cutSize_mm;

        if (maxLen <= 0) {
            ci.isManualCut = true;
        } else {
            double withComp = ci.cutSize_mm + comp;
            if (withComp > maxLen) {
                ci.isManualCut = true;
            } else {
                ci.effectiveCutSize_mm = withComp;
            }
        }
    }
}

inline QString buildMaterialStockReportForMachine_AUDIT(const MachineCuts& mc)
{
    QStringList out;
    out << "📦 Anyagfelhasználás (szálak anyagonként):";

    const CuttingMachine* machine =
        CuttingMachineRegistry::instance().findById(mc.machineHeader.machineId);

    if (!machine) {
        out << "⚠️ A gép nem található a CuttingMachineRegistry-ben.";
        return out.join("\n");
    }

    // 1) Stockból vágott szálak összegyűjtése
    QHash<QUuid, QSet<QString>> rodsByMaterial;
    for (const auto& ci : mc.cutInstructions) {
        if (ci.source == Cutting::Plan::Source::Stock)
            rodsByMaterial[ci.materialId].insert(ci.rodId);
    }

    if (rodsByMaterial.isEmpty()) {
        out << "ℹ️ Nincs stockból vágott szál.";
        return out.join("\n");
    }

    // 2) Tároló audit lekérése (modell)
    auto storageAudit = StorageAuditService::auditMachineStorage(*machine);
    //bool hasStorage = audit.hasStorage;
    //bool hasStockInStorage = audit.hasStockInStorage;
    //QVector<StorageAuditRow> auditRows = audit.rows;


    if (storageAudit.rows.isEmpty()) {
        if (!storageAudit.hasStorage) {
            out << "⚠️ A géphez nincs saját tároló rendelve.";
        } else {
            out << "ℹ️ A gép saját tárolója üres.";
        }
        // ettől függetlenül a szükséges anyag mennyisége kiszámolódik
    }


    // 3) Gép stockjának összesítése
    QHash<QUuid, int> stockByMaterial;
    for (const auto& row : storageAudit.rows)
        stockByMaterial[row.materialId] += row.actualQuantity;

    // 4) Anyagonkénti riport
    int totalNeeded = 0;
    int totalBringIn = 0;

    for (auto it = rodsByMaterial.begin(); it != rodsByMaterial.end(); ++it) {
        QUuid matId = it.key();
        int needed = it.value().size();
        int available = stockByMaterial.value(matId, 0);
        int bringIn = qMax(0, needed - available);

        totalNeeded += needed;
        totalBringIn += bringIn;

        const MaterialMaster* mat = MaterialRegistry::instance().findById(matId);
        QString matName = mat ? mat->toReportLabel() : QString("Material:%1").arg(matId.toString());

        double rodLength_m = mat && mat->stockLength_mm > 0
                                 ? mat->stockLength_mm / 1000.0
                                 : 0.0;

        out << QString("  • %1:").arg(matName);
        out << QString("      – szükséges: %1 szál (%2 m)")
                   .arg(needed)
                   .arg(QString::number(needed * rodLength_m, 'f', 2));
        out << QString("      – gép stockján: %1 szál").arg(available);
        out << QString("      – bevinni: %1 szál (%2 m)")
                   .arg(bringIn)
                   .arg(QString::number(bringIn * rodLength_m, 'f', 2));
    }

    // 5) Összesítő

    if (rodsByMaterial.size() > 1) {
        out << QString("📊 Összesen érintett szál: %1").arg(totalNeeded);
        out << QString("📦 Bevinni szükséges összesen: %1 szál").arg(totalBringIn);
    }

    return out.join("\n");
}

// A CutInstructions (MachineCuts) IGEN, gépenkénti
inline QString formatMachineCutsEvent(const MachineCuts& mc,
                                      const QString& planIdStr,
                                      const int printedLW)
{
    QStringList lines;

    QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

    int inputCount = 0;
    QStringList refs;
    for (const auto& ci : mc.cutInstructions) {
        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);
        if (req) {
            inputCount += 1;
            refs.append(ci.externalReference);
        }
    }

    QString compressed = TextHelper::compressRanges_String(refs);

    int outputCount = mc.cutInstructions.size();
    int diff = outputCount - inputCount;

    // --- fejlécek ---
    lines << "📄 Vágási utasítások (gépenkénti)";
    lines << QString("CutPlan: %1").arg(planIdStr);
    lines << QString("📅 Dátum: %1").arg(dateStr);
    lines << QString("⚙️ Gép: %1").arg(mc.machineHeader.machineName);
    lines << "──────────────────────────────────";
    lines << buildMaterialStockReportForMachine_AUDIT(mc);
    lines << "──────────────────────────────────";

    // --- hullók ---
    QSet<QString> usedLeftoverRods;
    for (const auto& ci : mc.cutInstructions)
        if (ci.source == Cutting::Plan::Source::Reusable)
            usedLeftoverRods.insert(ci.barcode);

    QStringList scrapList = usedLeftoverRods.values();
    if (!scrapList.isEmpty()) {
        lines << "♻️ Érintett hullók:";
        lines << QString("  • %1 db (%2)")
                     .arg(scrapList.size())
                     .arg(scrapList.join(", "));
        lines << "──────────────────────────────────";
    }

    // --- input/output ---
    lines << "📥 Gyártási input:";
    lines << QString("  • Kért darabszám: %1 db").arg(inputCount);
    lines << QString("  • Kért tételszámok: %1").arg(compressed);
    lines << "📤 Gyártási output:";
    lines << QString("  • Levágott darabok: %1 db").arg(outputCount);
    lines << QString("  • Eltérés: %1 db").arg(diff);
    lines << "──────────────────────────────────";

    // --- előkészítés ---
    QString prevRod;
    int maxStep = mc.cutInstructions.isEmpty()
                      ? 0
                      : mc.cutInstructions.last().globalStepId;

    int width = qMax(3, QString::number(maxStep).length());

    QVector<QString> sizeStrings;
    sizeStrings.reserve(mc.cutInstructions.size());
    QHash<QString,int> sizeCount;

    for (const auto& ci : mc.cutInstructions) {
        QString s = QString::number(ci.cutSize_mm, 'f', 1);
        sizeStrings.append(s);
        sizeCount[s] += 1;
    }

    // --- MODEL ---
    struct MachineCutsEvent_Row {
        QString colStepRod;
        QString colMaterial;
        QString colIconSizeCap;
        QString colPiece;
        QString colMult;
        QString capStr;   // kapocs karakter: "╖", "║", "╜", vagy ""
        QString rodId;
    };

    QVector<MachineCutsEvent_Row> rows;
    rows.reserve(mc.cutInstructions.size());

    QHash<QString,int> rodTotalCount;
    QHash<QString,int> rodSeenCount;

    for (const auto& ci : mc.cutInstructions) {
        QString rodKey = (ci.source == Cutting::Plan::Source::Reusable)
        ? ci.barcode
        : ci.rodId;
        rodTotalCount[rodKey] += 1;
    }

    QString prevSizeStr;
    QString prevSepAfterSize = "   ";
    int repeatCount = 1;
    bool firstOfBlock = true;
    int idx = 0;

    // --- MODEL FELTÖLTÉSE ---
    for (const auto& ci : mc.cutInstructions) {

        MachineCutsEvent_Row row;

        QString step = QString("%1.").arg(ci.globalStepId, width, 10, QLatin1Char(' '));

        QString rodIdOrBarcode = (ci.source == Cutting::Plan::Source::Reusable)
                                     ? ci.barcode
                                     : ci.rodId;

        int seen = rodSeenCount[rodIdOrBarcode]++;
        QString rodMarker = (seen == 0) ? " ●" : " ○";

        QString baseRodLabel = QString("%1 %2").arg(rodIdOrBarcode).arg(rodMarker);

        bool rodChanged = (ci.rodId != prevRod);
        QString rodLabel = rodChanged
                               ? QString("%1 □").arg(baseRodLabel)
                               : baseRodLabel + "  ";

        prevRod = ci.rodId;

        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(ci.materialId);

        QString materialLabel = mat
                                    ? QString("%1").arg(mat->barcode)
                                    : QString("Material:%1").arg(ci.materialId.toString(QUuid::WithoutBraces));

        QString sizeStr = sizeStrings[idx++];
        QString sizeFull = QString("%1 mm □").arg(sizeStr);

        bool sameAsPrev = (sizeStr == prevSizeStr);
        int count = sizeCount.value(sizeStr, 0);
        bool isRepeated = (count > 1);

        if (sameAsPrev) {
            repeatCount++;
            firstOfBlock = false;
        } else {
            repeatCount = 1;
            firstOfBlock = true;
        }

        QString sepAfterSize;
        QString sepLineConnector;
        QString capStr;

        if (!isRepeated) {
            sepAfterSize   = "   ";
            sepLineConnector = "   ";
            capStr         = "";
        } else if (firstOfBlock) {
            sepAfterSize   = " ╖ ";
            sepLineConnector = " ║ ";
            capStr         = "╖";
        } else if (repeatCount == count) {
            sepAfterSize   = " ╜ ";
            sepLineConnector = " ║ ";
            capStr         = "╜";
        } else {
            sepAfterSize   = " ║ ";
            sepLineConnector = " ║ ";
            capStr         = "║";
        }

        prevSizeStr = sizeStr;

        QString icon = ci.isManualCut ? "📏" : "✂️";

        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);
        QString pieceLabel = req
                                 ? QString("%1. %2").arg(ci.externalReference).arg(req->ownerName)
                                 : QString("req:%1").arg(ci.requestId.toString(QUuid::WithoutBraces));

        QString multiplier = "";
        if (isRepeated && firstOfBlock)
            multiplier = QString("  ×%1").arg(count);

        row.colStepRod     = step + " " + rodLabel;
        row.colMaterial    = materialLabel;
        row.colIconSizeCap = icon + " " + sizeFull + " " + sepAfterSize;
        row.colPiece       = pieceLabel;
        row.colMult        = multiplier;
        row.capStr = capStr;
        row.rodId = rodIdOrBarcode;
        rows.push_back(row);
    }

    // --- OSZLOPSZÉLESSÉGEK ---
    auto maxWidth = [&](auto getter){
        int m = 0;
        for (const auto& r : rows)
            m = qMax(m, getter(r).length());
        return m;
    };

    int wStepRod     = maxWidth([](auto& r){ return r.colStepRod; });
    int wMaterial    = maxWidth([](auto& r){ return r.colMaterial; });
    int wIconSizeCap = maxWidth([](auto& r){ return r.colIconSizeCap; });
    int wPiece       = maxWidth([](auto& r){ return r.colPiece; });
    int wMult        = maxWidth([](auto& r){ return r.colMult; });

    // --- FIX OSZLOPINDEXEK ---
    int colStepRodPos     = 0;
    int colMaterialPos    = colStepRodPos + wStepRod + 3;
    int colIconSizeCapPos = colMaterialPos + wMaterial + 3;
    int colPiecePos       = colIconSizeCapPos + wIconSizeCap + 3;
    int colMultPos        = colPiecePos + wPiece + 3;
    int colCheckboxPos = colPiecePos + wPiece + 1;

    // --- PIECE TRUNCÁLÁSA ---
    int maxPieceLen = printedLW - colPiecePos - 3;
    if (maxPieceLen < 5)
        maxPieceLen = 5;

    for (auto& r : rows) {
        if (r.colPiece.length() > maxPieceLen)
            r.colPiece = r.colPiece.left(maxPieceLen - 1) + "…";
    }

    // --- HELPER ---
    auto put = [&](QString& line, int pos, const QString& text){
        for (int i = 0; i < text.length() && pos + i < line.length(); ++i)
            line[pos + i] = text[i];
    };

    auto writeSeparator = [&](const MachineCutsEvent_Row& r){

        // ha nincs kapocs vagy lezáró kapocs → nincs szeparátor
        // if (r.capStr.isEmpty() || r.capStr == "╜")
        //     return;

        //int sepLen = colMultPos + wMult;
        int sepLen = colIconSizeCapPos + wIconSizeCap;

        QString sep(sepLen, ' ');

        // az első oszlophatárig vízszintes vonal
        for (int i = 0; i < colMaterialPos; ++i)
            sep[i] = u'─';

        //sep[colMaterialPos - 1]    = '|';
        // sep[colIconSizeCapPos - 1] = '|';
        // sep[colPiecePos - 1]       = '|';
        // sep[colMultPos - 1]        = '|';

        // kapocs folytatási szabály
        QChar sepCap = ' ';
        if (r.capStr == "╖") sepCap = u'║';
        else if (r.capStr == "║") sepCap = u'║';
        else sepCap = ' '; // "╜" vagy ""

        if (sepCap != ' ') {

            int capPos = colIconSizeCapPos
                         + r.colIconSizeCap.length()
                         - r.capStr.length() - 1;

            if (capPos >= 0 && capPos < sep.length())
                sep[capPos] = sepCap;
        }

        lines << sep;
    };


    // --- KIÍRÁS ---
    bool first = true;
    const MachineCutsEvent_Row* prevRow = nullptr;
    for (const auto& r : rows) {

        if (prevRow != nullptr && r.rodId != prevRow->rodId) {
            writeSeparator(*prevRow);   // ⭐ előző sor kapcsa
        }

        first = false;

        QString line(printedLW, ' ');

        put(line, colStepRodPos,     r.colStepRod);
        put(line, colMaterialPos,    r.colMaterial);
        put(line, colIconSizeCapPos, r.colIconSizeCap);
        put(line, colPiecePos,       r.colPiece);

        // checkbox külön oszlopban
        if (colCheckboxPos < line.length())
            line[colCheckboxPos] = u'□';

        put(line, colMultPos,        r.colMult);

        line[colMaterialPos - 1]    = '|';
        line[colIconSizeCapPos - 1] = '|';
        //line[colPiecePos - 1]       = '|';
        //line[colMultPos - 1]        = '|';

        lines << line;

            prevRow = &r;   // ⭐ frissítjük az előző sort
    }

    return lines.join("\n");
}



// struct LabelPart {
//     QString text;

//     bool trimmable = false;   // rövidíthető
//     bool jumpable = false;    // kiugorhat új sorba
//     int targetRow = 0;        // melyik sorba szeretnénk alapból
//     Qt::Alignment align = Qt::AlignLeft;  // bal/közép/jobb
// };


// struct LabelModel {
//     QVector<LabelPart> parts;
//     QString priorityIcon;   // 🔥💧☁️🪨
//     QString groupIcon;      // 🦌🐸🐱… ABC állatok

//     QString toString() const {
//         QString out;
//         for (const auto& p : parts)
//             out += p.text;
//         return out;
//     }

//     int length() const {
//         int len = 0;
//         for (const auto& p : parts)
//             len += p.text.length();
//         return len;
//     }
// };

// PRIORITY‑ICON BUCKETING, PRIORITY ICON SCALE
// Lean / Kanban / Toyota Production System egyik alapelve:
// A prioritás vizuális, ikonikus, azonnal felismerhető legyen.



inline QString priorityIconFor(const QDate& dueDate)
{
    if (!dueDate.isValid())
        return EmojiHelper::priorityIconFor(-1);   // nincs határidő → nincs prio

    QDate today = QDate::currentDate();
    int daysLeft = today.daysTo(dueDate);

    return EmojiHelper::priorityIconFor(daysLeft);
}

// ikonos batch‑azonosítás (Toyota, Bosch, Siemens)


inline QMap<QString, QString> computeGroupIconsForRequests(const QVector<Cutting::Plan::Request>& reqs)
{
    QMap<QString, QString> out;

    // 1) stabil rendezés
    QVector<Cutting::Plan::Request> sorted = reqs;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b){
                  if (a.ownerName != b.ownerName)
                      return a.ownerName < b.ownerName;
                  return a.externalReference < b.externalReference;
              });

    // 2) rotációs állapot
    int groupIndex = -1;
    int groupCount = 0;
    QString prevOwner;

    const int MAX_GROUP_SIZE = 10;

    // 3) végigmegyünk a rendezett requesteken
    for (const auto& r : sorted) {

        QString owner = r.ownerName;
        QString ext   = r.externalReference;

        // megrendelőváltás → új ikon
        if (owner != prevOwner) {
            groupIndex++;
            groupCount = 0;
        }

        // túl nagy csoport → új ikon
        if (groupCount >= MAX_GROUP_SIZE) {
            groupIndex++;
            groupCount = 0;
        }

        // ikon kiválasztása
        QString icon = EmojiHelper::getGroupIcon(groupIndex);

        out[ext] = icon;

        groupCount++;
        prevOwner = owner;
    }

    return out;
}

inline QVector<LabelModel> collectLabelModelsFromMachineCuts(const MachineCuts& mc)
{
    QVector<LabelModel> out;
    QSet<QString> rodSeen;
    QVector<Cutting::Plan::Request> reqs;

    for (const auto& ci : mc.cutInstructions) {
        Cutting::Plan::Request *req =
            CuttingPlanRequestRegistry::instance().findById(ci.requestId);
        if(req)
            reqs.append(*req);
    }

    QMap<QString, QString> groupIcons = computeGroupIconsForRequests(reqs);

    for(auto& g : groupIcons.keys()){
        zInfo(L("Group icon: %1 → %2").arg(g).arg(groupIcons[g]));
    }

    for (const auto& ci : mc.cutInstructions) {

        // 1) Rúd címke
        if (!rodSeen.contains(ci.rodId)) {
            rodSeen.insert(ci.rodId);

            QString rodIdOrBarcode = (ci.source == Cutting::Plan::Source::Reusable)
                                         ? ci.barcode
                                         : ci.rodId;

            LabelModel rod;
            rod.parts.append({
                rodIdOrBarcode,
                false, false,
                0,
                Qt::AlignCenter
            });

            const MaterialMaster* mat =
                MaterialRegistry::instance().findById(ci.materialId);

            if(mat){
                rod.parts.append({
                    ":"+mat->toDisplay(), //toReportLabel(),
                    false,      // trimmable
                    false,      // jumpable
                    1,          // targetRow → alsó sor
                    Qt::AlignCenter,
                    true        // 🔥 small = true
                });
            }

            //rod.groupIcon = "🌞";
            //rod.priorityIcon = "🌞";


            out.append(rod);
        }

        // 2) Darab címke
        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);
        QString ext = ci.externalReference + ".";

        // 🔥💧☁️⏳ prioritás ikon
        QString prio = req ? priorityIconFor(req->dueDate) : "🌞";

        // 🐸🐱🦭… csoportikon
        QString baseRef = ci.externalReference.split(' ').first();
        QString group = groupIcons.value(baseRef, "🐞");

        // zInfo(QStringLiteral("prio(%1) -> %2").arg(req->dueDate.toString()).arg(prio));
        // zInfo(QStringLiteral("group(%1)-> %2").arg(ci.externalReference).arg(group));

        QString owner = req ? req->ownerName
                            : ci.requestId.toString(QUuid::WithoutBraces);
        QString sizeStr = QString("%1 mm").arg(QString::number(ci.cutSize_mm, 'f', 0));

        LabelModel lm;
        lm.priorityIcon = prio;
        lm.groupIcon = group;
        lm.barcode = ci.externalReference;

        //lm.parts.append({ ext + " ", false, false, 0, Qt::AlignLeft });
        //lm.parts.append({ prio + group + " " + ext + " ", false, false, 0, Qt::AlignLeft });
        lm.parts.append({ ext + " ", false, false, 0, Qt::AlignLeft });
        lm.parts.append({ owner,     true,  true,  0, Qt::AlignCenter });

        // QString a = "";
        // if(ci.subtype != Subtype::None){
        //     a+= SubtypeUtils::toDisplayText(ci.subtype);
        // }
        QString a;
/*        a = SubtypeUtils::toProductVariantDisplayText(ci.productTypeId,
                                                      ci.productSubtypeId,
                                                      ci.attributes);
*/
        auto* b = ProductSubtypeRegistry::instance().findById(ci.productSubtypeId);
        a = b?b->name:"";

        if(ci.side != HandlerSide::None) {
            if(!a.isEmpty()) a += ", ";
            a += HandlerSideUtils::toDisplayText(ci.side);;
        }

        if(!a.isEmpty()){
            lm.parts.append({ " ("+a+")",   false, false, 0, Qt::AlignRight });
        }
        lm.parts.append({ " | "+sizeStr,   false, false, 0, Qt::AlignRight });
        out.append(lm);
    }

    return out;
}



inline void trimLabelToWidth(LabelModel& lm, int maxWidth)
{
    int L = lm.length();
    if (L <= maxWidth)
        return;

    // 1) fix és trimmable részek szétválasztása
    int fixedLen = 0;
    QVector<int> trimIndices;

    for (int i = 0; i < lm.parts.size(); ++i) {
        if (lm.parts[i].trimmable)
            trimIndices.append(i);
        else
            fixedLen += lm.parts[i].text.length();
    }

    // 2) nincs trimmable rész → kényszer-trimmelés
    if (trimIndices.isEmpty()) {
        QString& t = lm.parts.last().text;
        int keep = qMax(1, maxWidth - 1);
        t = t.left(keep) + "…";
        return;
    }

    // 3) mennyi hely jut összes trimmable részre
    int available = maxWidth - fixedLen;
    if (available < (int)trimIndices.size())
        available = trimIndices.size(); // minden trimmable résznek legalább 1 hely

    // 4) összes trimmable hossz
    int totalTrimLen = 0;
    for (int i : trimIndices)
        totalTrimLen += lm.parts[i].text.length();

    // 5) ha belefér → kész
    if (totalTrimLen <= available)
        return;

    // 6) szükséges vágás
    int need = totalTrimLen - available;

    // 7) trimmable részek rendezése hossz szerint
    std::sort(trimIndices.begin(), trimIndices.end(),
              [&](int a, int b){
                  return lm.parts[a].text.length() > lm.parts[b].text.length();
              });

    // 8) vágás a leghosszabbtól lefelé
    for (int idx : trimIndices) {
        QString& t = lm.parts[idx].text;

        int oldLen = t.length();
        if (oldLen <= 1)
            continue;

        // ennyit vághatunk maximum ebből a részből
        int canCut = oldLen - 1; // legalább 1 hely maradjon (karakter + ellipsis)

        if (canCut <= 0)
            continue;

        int cut = qMin(canCut, need);

        // vágás
        t = t.left(oldLen - cut);

        bool hadEllipsis = t.endsWith("…");
        if (!hadEllipsis)
            t += "…";

        int newLen = t.length();
        int actualReduction = oldLen - newLen;   // ez a VALÓDI hosszcsökkenés

        need -= actualReduction;
        if (need <= 0)
            break;
    }

    // 9) ha még mindig maradt need → kényszer-trimmelés
    // ha valamiért még mindig hosszabb (numerikus kerekítés, több ellipsis, stb.)
    // akkor kényszerrel levágjuk az utolsó trimmable részt úgy, hogy biztosan beleférjen
    if (lm.length() > maxWidth) {
        int idx = trimIndices.last();
        QString& t = lm.parts[idx].text;

        int over = lm.length() - maxWidth;
        int keep = qMax(1, t.length() - over - 1); // -1 az ellipsisnek
        t = t.left(keep) + "…";
    }
}


inline QString makeHLine(int count)
{
    const QChar h = QStringLiteral("─").at(0);
    QString s;
    s.reserve(count);
    for (int i = 0; i < count; ++i)
        s.append(h);
    return s;
}

inline QString makeSpaces(int count)
{
    const QChar sp = QStringLiteral(" ").at(0);
    QString s;
    s.reserve(count);
    for (int i = 0; i < count; ++i)
        s.append(sp);
    return s;
}

inline QString trimToWidth(const QString& s, int maxWidth)
{
    if (s.length() <= maxWidth)
        return s;

    // 1 karakter hely kell az ellipsisnek
    return s.left(maxWidth - 1) + QStringLiteral("…");
}

inline QString makeLabel(const QString& externalRef,
                         const QString& ownerName,
                         double size_mm,
                         int maxWidth)
{
    QString sizeStr = QString("%1 mm").arg(size_mm, 0, 'f', 0);

    // mennyi hely jut az ownerName-nek?
    int ownerMax = maxWidth
                   - externalRef.length()
                   - 1               // szóköz externalRef után
                   - 3               // " | "
                   - sizeStr.length();

    if (ownerMax < 1)
        ownerMax = 1;

    // csak az ownerName trimmelődik
    QString trimmedOwner = trimToWidth(ownerName, ownerMax);

    // a teljes címke már NEM trimmelődik
    return QString("%1 %2 | %3")
        .arg(externalRef)
        .arg(trimmedOwner)
        .arg(sizeStr);
}

inline QString makeMiddleBorder(int cols, int cellWidth)
{
    QString mid = QStringLiteral("├");
    for (int c = 0; c < cols; ++c) {
        mid += makeHLine(cellWidth);
        mid += (c == cols - 1 ? QStringLiteral("┤") : QStringLiteral("┼"));
    }
    return mid;
}

// --- buildLabelCellLines: címke cella 1–2 soros felépítése ---
inline QVector<QString> buildLabelCellLines(const QVector<LabelPart>& parts,
                                            int cellWidth)
{
    QVector<QString> lines;

    // BELTÉR – ezzel kell dolgozni
    const int W = qMax(1, cellWidth - 2);

    QString leftZone, centerZone, rightZone;

    for (const auto& p : parts) {
        if (p.align == Qt::AlignLeft)      leftZone += p.text;
        else if (p.align == Qt::AlignCenter) centerZone += p.text;
        else if (p.align == Qt::AlignRight)  rightZone += p.text;
    }

    int L = leftZone.length();
    int C = centerZone.length();
    int R = rightZone.length();

    // 1) minden befér
    if (L + C + R <= W) {
        int free = W - (L + R);
        int leftPad = (free - C) / 2;
        int rightPad = free - C - leftPad;

        QString line =
            leftZone +
            QString(leftPad, ' ') +
            centerZone +
            QString(rightPad, ' ') +
            rightZone;

        lines.append(line.left(W));
        return lines;
    }

    // 2) center jumpable
    if (L + C + R > W && L + R <= W) {
        int free = W - (L + R);
        if (free < 0) free = 0;

        QString line1 =
            leftZone +
            QString(free, ' ') +
            rightZone;

        lines.append(line1.left(W));

        QString c = centerZone;
        if (c.length() > W)
            c = c.left(W - 1) + "…";

        int padL = (W - c.length()) / 2;
        int padR = W - c.length() - padL;

        QString line2 =
            QString(padL, ' ') +
            c +
            QString(padR, ' ');

        lines.append(line2.left(W));
        return lines;
    }

    // 3) left + right sem fér → bal trimmelése
    if (L + R > W) {
        int maxLeft = W - R;
        if (maxLeft < 1) maxLeft = 1;

        QString trimmedLeft = leftZone;
        if (trimmedLeft.length() > maxLeft)
            trimmedLeft = trimmedLeft.left(maxLeft - 1) + "…";

        int L2 = trimmedLeft.length();
        int free = W - (L2 + R);
        if (free < 0) free = 0;

        QString line =
            trimmedLeft +
            QString(free, ' ') +
            rightZone;

        lines.append(line.left(W));
        return lines;
    }

    // fallback
    {
        int free = W - (L + R);
        if (free < 0) free = 0;

        QString line =
            leftZone +
            QString(free, ' ') +
            rightZone;

        lines.append(line.left(W));
        return lines;
    }
}


inline QString makeContentRow(const QString& text, int cellWidth)
{
    int inner = cellWidth - 2;

    QString T = text;
    if (T.length() > inner)
        T = T.left(inner);

    T = T.leftJustified(inner, ' ');

    return "|" + QStringLiteral(" ") + T + QStringLiteral(" ") + "|";
}


inline QString makePaddingRow(int cellWidth)
{
    QString pad = QStringLiteral(" ").repeated(cellWidth);
    return "|" + pad + "|";
}

inline QString makeSeparatorRow(int cellWidth)
{
    QString sep = QStringLiteral("─").repeated(cellWidth);
    return "|" + sep + "|";
}

// két oszlop összeolvasztása → |AAA|BBB|
inline QString mergeTwo(const QString& left, const QString& right)
{
    return left + right.mid(1);
}

inline QStringList renderColumn(const QVector<QStringList>& boxes, int cellWidth, int columnIndex)
{
    QStringList out;

    // --- 0) Oszlopszám címke ---
    {
        int inner = cellWidth - 2;
        QString num = QString::number(columnIndex);
        int padL = (inner - num.length()) / 2;
        int padR = inner - num.length() - padL;

        QString line = QStringLiteral("|") +
                       " " +
                       QString(padL, ' ') + num + QString(padR, ' ') +
                       " " +
                       "|";

        out << makeSeparatorRow(cellWidth);
        out << line;
    }

    // teljes oszlop teteje
    out << makeSeparatorRow(cellWidth);

    for (int i = 0; i < boxes.size(); ++i) {
        const auto& b = boxes[i];

        // top padding
        out << makePaddingRow(cellWidth);

        // content
        for (auto& line : b)
            out << makeContentRow(line, cellWidth);

        // bottom padding
        out << makePaddingRow(cellWidth);

        // cellák közti szeparátor
        out << makeSeparatorRow(cellWidth);
    }

    return out;
}

inline QString formatLabelColumnFlow(const QVector<LabelModel>& models,
                                     int pageWidth,
                                     int pageHeight,
                                     int columns,
                                     int headerLines)
{
    // cellWidth képlete az összeolvadó keretekhez + gaphez
    int cellWidth = (pageWidth - 3*columns + 1) / columns;

    QVector<LabelModel> m2;
    for(auto&model:models){
        if(model.parts.isEmpty())
            continue;
        LabelModel m = model;

        QString txt1 ="";
        if(!m.priorityIcon.isEmpty())
            txt1+=m.priorityIcon;
        if(!m.groupIcon.isEmpty())
            txt1+=m.groupIcon;

        if(!txt1.isEmpty()){
            for(auto&p : m.parts){
                if(p.align== Qt::AlignLeft){
                    p.text = txt1 + " " + p.text;
                    break;
                }
            }
            //model.parts[0] = txt1 + " " + model.parts[0];
        }
        m2.append(m);
    }

    QVector<QStringList> boxes;

    // 1) minden címke külön tartalom (keret nélkül)
    for (auto& m : m2){
//        QVector<QString> box;

//        QString txt1 ="";
//        if(!m.priorityIcon.isEmpty())
//            txt1+=m.priorityIcon;
//        if(!m.groupIcon.isEmpty())
//            txt1+=m.groupIcon;

        boxes << buildLabelCellLines(m.parts, cellWidth);

//        if(!txt1.isEmpty() && !box.isEmpty()){
//            box[0] = txt1 + " " + box[0];
//        }

//        boxes << box;
    }

    // 2) oszlopfolytonos tördelés (függőleges)
    QVector<QVector<QStringList>> cols;
    QVector<QStringList> col;

    int currentPage = 0;
    int h = 0;

    for (auto& b : boxes) {

        int availableHeight = (currentPage == 0)
        ? (pageHeight - headerLines)
        : pageHeight;

        int bh = b.size() + 2; // padding miatt

        if (h + bh > availableHeight) {
            cols << col;
            col.clear();
            h = 0;
            currentPage++;
        }

        col << b;
        h += bh + 1; // + szeparátor
    }

    if (!col.isEmpty())
        cols << col;

    // 3) lapfolytonos tördelés (vízszintes)
    QVector<QVector<QVector<QStringList>>> pages;
    QVector<QVector<QStringList>> page;

    for (int i = 0; i < cols.size(); ++i) {
        if (page.size() == columns) {
            pages << page;
            page.clear();
        }
        page << cols[i];
    }
    if (!page.isEmpty())
        pages << page;

    // 4) oldalak renderelése
    QStringList out;

    int cIndex = 1;
    for (int p = 0; p < pages.size(); ++p) {

        auto& pageCols = pages[p];

        QVector<QStringList> rendered;
        for (auto& c : pageCols)
            rendered << renderColumn(c, cellWidth, cIndex++);

        int maxRows = 0;
        for (auto& r : rendered)
            maxRows = qMax(maxRows, r.size());

        for (int r = 0; r < maxRows; ++r) {

            QString line;

            for (int c = 0; c < rendered.size(); ++c) {

                QString cell = (r < rendered[c].size())
                ? rendered[c][r]
                : makePaddingRow(cellWidth);

                if (c == 0)
                    line = cell;
                else
                    line = mergeTwo(line, cell);
            }

            out << line;
        }

        // lapok között csak üres sor, szeparátor NEM kell
        // if (p + 1 < pages.size())
        //     out << "\f"; // form feed - új oldal
        // lapok között form feed, de csak a második laptól
        if (p > 0)
            out << "\f";

    }

    return out.join("\n");
}


inline QString formatLeftoverIntakeForm_OnePage(int pageWidth, int rowsPerPage)
{
    // 1) Oszlopszélességek (4 oszlop + 5 db '|' + 3*1 szóköz)
    int innerWidth = pageWidth - 5; // 4 oszlop + 1 záró '|' → nagyjából
    if (innerWidth < 40)
        innerWidth = 40; // minimális biztonsági érték

    int col1 = innerWidth * 30 / 100; // Material barcode (kézzel)
    int col2 = innerWidth * 15 / 100; // Length (kézzel)
    int col3 = innerWidth * 25 / 100; // RSM kód (nyomtatott)
    int col4 = innerWidth - (col1 + col2 + col3); // Cut-off label

    auto makeCell = [](const QString& text, int w) {
        QString t = text;
        if (t.length() > w)
            t = t.left(w - 1) + "…";
        return t.leftJustified(w, ' ');
    };

    QStringList lines;

    // 2) Fejléc
    lines << QString("📝 Leftover felvételi űrlap (manual RSM címkék)");
    lines << QString("📅 Dátum: %1").arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm"));
    lines << "";

    // 3) Táblázat fejléce
    QString header1 =
        "|" + makeCell("Material", col1) +
        "|" + makeCell("Length",       col2) +
        "|" + makeCell("LeftoverStock",          col3) +
        "|" + makeCell("Cut-off label",     col4);
    QString header2 =
        "|" + makeCell("barcode", col1) +
        "|" + makeCell("[mm]",       col2) +
        "|" + makeCell("barcode",          col3) +
        "|" + makeCell("",     col4);
    lines << header1 << header2;

    QString sep =
        "|" + QString(col1, '-') +
        "|" + QString(col2, '-') +
        "|" + QString(col3, '-') +
        "|" + QString(col4, '-');
    lines << sep;

    // 4) RSM kódok generálása – PEEK alapú, COMMIT NÉLKÜL
    int next = SettingsManager::instance().peekManualLeftoverCounter();

    for (int i = 0; i < rowsPerPage; ++i){
        QString code;
        while(true){
            code = IdentifierUtils::makeManualLeftoverId(next++);

            // és azt kell itt megnézni, hogy a code az létezik-e már, mert ha igen, át kell ugrani
            // ahol felajánlja a dialogban a köv. kódot, az pont jó
            // 🔥 Barcode egyediség ellenőrzése
            bool exists = LeftoverStockRegistry::instance().existsBarcode(code, {});

            if (!exists)
                break;
        }
        // 1) Üres felső sor
        QString rowTop =
            "|" + makeCell("", col1) +
            "|" + makeCell("", col2) +
            "|" + makeCell("", col3) +
            "|" + makeCell("", col4);

        // 2) Középre rendezett RSM kód
        auto centerCell = [&](const QString& text, int w){
            int pad = (w - text.length()) / 2;
            if (pad < 0) pad = 0;
            return QString(pad, ' ') + text + QString(w - pad - text.length(), ' ');
        };

        QString rowMid =
            "|" + makeCell("", col1) +
            "|" + makeCell("", col2) +
            "|" + centerCell(code, col3) +
            "|" + centerCell(code, col4);

        // 3) Üres alsó sor
        QString rowBot =
            "|" + makeCell("", col1) +
            "|" + makeCell("", col2) +
            "|" + makeCell("", col3) +
            "|" + makeCell("", col4);

        lines << rowTop;
        lines << rowMid;
        lines << rowBot;

        // 4) Szeparátor
        QString sepRow =
            "|" + QString(col1, QChar(0x2500)) +
            "|" + QString(col2, QChar(0x2500)) +
            "|" + QString(col3, QChar(0x2500)) +
            "|" + QString(col4, QChar(0x2500));

        lines << sepRow;

        // 🔥 ÚJ: sorok közötti szeparátor
        // QString sepRow =
        //     "|" + QString(col1, QChar(0x2500)) +
        //     "|" + QString(col2, QChar(0x2500)) +
        //     "|" + QString(col3, QChar(0x2500)) +
        //     "|" + QString(col4, QChar(0x2500)) +
        //     "|";

        // lines << sepRow;
    }

    return lines.join("\n");
}

struct RenderLine {
    QVector<LabelPart> left;
    QVector<LabelPart> center;
    QVector<LabelPart> right;
};

inline QVector<RenderLine> buildRenderLines(
    const QVector<LabelPart>& parts,
    QFontMetrics& fm,
    qreal contentWidth)
{
    QVector<RenderLine> out;

    // --- 1) Zónák szétválogatása ---
    QVector<LabelPart> leftParts;
    QVector<LabelPart> centerParts;
    QVector<LabelPart> rightParts;

    for (const auto& p : parts) {
        if (p.align == Qt::AlignLeft)
            leftParts.append(p);
        else if (p.align == Qt::AlignCenter)
            centerParts.append(p);
        else if (p.align == Qt::AlignRight)
            rightParts.append(p);
    }

    auto widthOf = [&](const LabelPart& p){
        return fm.horizontalAdvance(p.text);
    };

    auto sumWidth = [&](const QVector<LabelPart>& v){
        qreal w = 0;
        for (auto& p : v) w += widthOf(p);
        return w;
    };

    qreal leftW   = sumWidth(leftParts);
    qreal centerW = sumWidth(centerParts);
    qreal rightW  = sumWidth(rightParts);

    // --- 2) Ha minden befér egy sorba ---
    if (leftW + centerW + rightW <= contentWidth) {
        RenderLine line;
        line.left   = leftParts;
        line.center = centerParts;
        line.right  = rightParts;
        out.append(line);
        return out;
    }

    // --- 3) Ha center nem fér → center külön sorokba kerül ---
    if (leftW + rightW <= contentWidth) {

        // Sor 1: bal + jobb
        RenderLine line1;
        line1.left  = leftParts;
        line1.right = rightParts;
        out.append(line1);

        // Sor 2..N: minden center külön sor
        for (const auto& cp : centerParts) {
            RenderLine cl;
            cl.center.append(cp);
            out.append(cl);
        }

        return out;
    }

    // --- 4) Bal + jobb sem fér → bal trimmelése ---
    qreal maxLeft = contentWidth - rightW;
    if (maxLeft < 20) maxLeft = 20;

    QVector<LabelPart> trimmedLeft;
    qreal used = 0;

    for (auto& p : leftParts) {
        qreal w = widthOf(p);
        if (used + w <= maxLeft) {
            trimmedLeft.append(p);
            used += w;
        } else {
            LabelPart ell = p;
            ell.text = "…";
            trimmedLeft.append(ell);
            break;
        }
    }

    RenderLine line;
    line.left  = trimmedLeft;
    line.right = rightParts;
    out.append(line);

    return out;
}

// inline int measureGlyphHeight(const QFont& font)
// {
//     QImage img(200, 200, QImage::Format_ARGB32);
//     img.fill(Qt::transparent);

//     QPainter p(&img);
//     p.setFont(font);
//     p.setPen(Qt::black);

//     // Kirajzolunk egy nagy, magas karaktert
//     QString ch = "W";

//     // Kirajzoljuk 0,0-tól
//     p.drawText(0, 150, ch);
//     p.end();

//     // Pixelben lemérjük a nem-átlátszó pixelek tartományát
//     int top = 200, bottom = 0;

//     for (int y = 0; y < img.height(); ++y) {
//         for (int x = 0; x < img.width(); ++x) {
//             if (qAlpha(img.pixel(x, y)) > 0) {
//                 top = qMin(top, y);
//                 bottom = qMax(bottom, y);
//             }
//         }
//     }

//     return bottom - top + 1;
// }

// #include <QPdfWriter>
// #include <QPdfDocument>
// #include <QBuffer>
// #include <QPainter>

// qreal measurePdfGlyphHeight(const QFont& font)
// {
//     // 1) PDF memóriába
//     QBuffer buffer;
//     buffer.open(QIODevice::WriteOnly);

//     QPdfWriter pdf(&buffer);
//     pdf.setPageSize(QPageSize(QPageSize::A4));
//     pdf.setResolution(72); // PDF natív DPI

//     QPainter painter(&pdf);
//     painter.setFont(font);

//     // 2) Kirajzolunk egy karaktert
//     QString ch = "W";
//     painter.drawText(100, 200, ch);
//     painter.end();

//     // 3) PDF visszaolvasása
//     QPdfDocument doc;
//     doc.load(buffer.data());

//     // 4) Bounding box kiolvasása
//     // A teljes oldalra keresünk szöveget
//     QRectF bbox = doc.getPage(0)->textSelectionBoundingBox(
//         QPointF(0, 0),
//         QPointF(1000, 1000)
//         );

//     return bbox.height();
// }



inline void formatLeftoverIntakeForm_Pdf(
    QPainter& painter,
    QPdfWriter& writer,
    const QRectF& pageRect,
    int rowsPerPage
    ){
    QFontMetrics fm(painter.font());
    qreal lineH = fm.height() ;//* 1.45;
    const qreal topMargin = 40.0;
    const qreal leftMargin = 40.0;
    const qreal barcodeHeight0 = 80.0;
    const qreal gap = 8.0;

    qreal sidePad = lineH;//4.0;

    qreal y = topMargin;

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(QPen(Qt::black, 0.75));


    // 1) Fejléc
    painter.drawText(QRectF(leftMargin, y, pageRect.width(), lineH),
                     Qt::AlignLeft,
                     "📝 Leftover felvételi űrlap (manual RSM címkék)");
    y += lineH;

    painter.drawText(QRectF(leftMargin, y, pageRect.width(), lineH),
                     Qt::AlignLeft,
                     QString("📅 Dátum: %1")
                         .arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm")));
    y += lineH; // 1 sor + kis gap
    y += gap;

    // 2) Oszlopszélességek
    qreal totalW = pageRect.width() - leftMargin * 2;
    qreal col1 = totalW * 0.30;
    qreal col2 = totalW * 0.15;
    qreal col3 = totalW * 0.25;
    qreal col4 = totalW - (col1 + col2 + col3);

    qreal xCol1 = leftMargin;
    qreal xCol2 = leftMargin + col1;
    qreal xCol3 = leftMargin + col1 + col2;
    qreal xCol4 = leftMargin + col1 + col2 + col3;

    auto drawRow = [&](qreal yRow,
                       const QString& t1,
                       const QString& t2,
                       const QString& t3,
                       const QString& t4,
                       bool center = false)
    {
        auto align = center?Qt::AlignCenter:Qt::AlignLeft;
        qreal textPad = lineH;// * 0.4;

        painter.drawText(QRectF(xCol1 + textPad, yRow, col1 - textPad, lineH), align, t1);
        painter.drawText(QRectF(xCol2 + textPad, yRow, col2 - textPad, lineH), align, t2);
        painter.drawText(QRectF(xCol3 + textPad, yRow, col3 - textPad, lineH), align, t3);
        painter.drawText(QRectF(xCol4 + textPad, yRow, col4 - textPad, lineH), align, t4);
    };

    auto drawTableFrame = [&](qreal top, qreal bottom, bool topheader = false)
    {
        // vízszintes vonalak
        if(topheader)
            painter.drawLine(leftMargin, top,    leftMargin + totalW, top);
        painter.drawLine(leftMargin, bottom, leftMargin + totalW, bottom);

        // függőleges vonalak
        painter.drawLine(xCol1, top, xCol1, bottom);
        painter.drawLine(xCol2, top, xCol2, bottom);
        painter.drawLine(xCol3, top, xCol3, bottom);
        painter.drawLine(xCol4, top, xCol4, bottom);
        //painter.drawLine(leftMargin + totalW, top, leftMargin + totalW, bottom);
    };

    qreal headerTop = y;
    y += lineH/2;
    // 3) Táblázat fejléce
    drawRow(y, "Material", "Length", "LeftoverStock", "Cut-off label");
    y += lineH;
    drawRow(y, "barcode", "[mm]", "barcode", "");
    y += lineH;
    y += lineH/2;
    qreal headerBottom = y;

    drawTableFrame(headerTop, headerBottom, true);

//    painter.drawLine(leftMargin, y, leftMargin + totalW, y);


    // 4) RSM kód generálás
    int next = SettingsManager::instance().peekManualLeftoverCounter();

    for (int i = 0; i < rowsPerPage; ++i) {

        // --- egyedi RSM kód keresése ---
        QString code;
        while (true) {
            code = IdentifierUtils::makeManualLeftoverId(next++);
            if (!LeftoverStockRegistry::instance().existsBarcode(code, {}))
                break;
        }

        // Blokk (1 leftover) magassága: 3 sor + szeparátor-gap
        //qreal blockHeight = lineH * 3;
        qreal blockHeight = lineH + lineH + barcodeHeight0;
        // lineH = szöveg, gap = szöveg–barcode, 8 = padding

        qreal cellTop = y;
        qreal cellBottom = cellTop + blockHeight;

        // 1) felső üres sor
        //drawRow(cellTop + 0 * lineH, "", "", "", "");

        // 2) középső sor – szöveg a 3. és 4. oszlopban
        drawRow(cellTop + lineH/2+gap, "", "", code, code, true);
        //drawRow(cellTop + 1 * lineH, "", "", "", "");

        // barcode panel a 3. oszlopban
        //qreal textY = cellTop + 1 * lineH;
        //QRectF textRect(xCol3, textY, col3, lineH);
        //painter.drawText(textRect, Qt::AlignCenter, code);

        // extra padding a barcode körül
        qreal topPad = 0.0;
        qreal bottomPad = 0.0;


        qreal barcodeTop = cellTop + lineH + lineH + gap + topPad;
        qreal barcodeBottom = cellBottom - bottomPad;

        qreal available = barcodeBottom - barcodeTop;
        if (available < 0) available = 0;

        qreal barcodeH = qMin(available, barcodeHeight0);
        qreal barcodeY = barcodeBottom - barcodeH;


        BarcodePainter::drawCode128(
            painter,
            code,
            QRectF(xCol3 + sidePad, barcodeY, col3-2*sidePad, barcodeH)
            );

        // barcode panel a 4. oszlopban is
        //QRectF textRect4(xCol4, textY, col4, lineH);
        //painter.drawText(textRect4, Qt::AlignCenter, code);

        const qreal glueMargin = 90.0;   // ragacs margó

        BarcodePainter::drawCode128(
            painter,
            code,
            QRectF(xCol4 + sidePad +glueMargin, barcodeY, col4-2*sidePad-glueMargin, barcodeH)
            );

        // 3) alsó üres sor
        //drawRow(cellTop + 2 * lineH, "", "", "", "");

        // vízszintes szeparátor a blokk alján
        // painter.drawLine(leftMargin, cellBottom, leftMargin + totalW, cellBottom);

        // // függőleges szeparátorok
        // painter.drawLine(xCol1, cellTop, xCol1, cellBottom);
        // painter.drawLine(xCol2, cellTop, xCol2, cellBottom);
        // painter.drawLine(xCol3, cellTop, xCol3, cellBottom);
        // painter.drawLine(xCol4, cellTop, xCol4, cellBottom);

        drawTableFrame(cellTop, cellBottom);

        // 🔥 Ragasztócsík jelölése a Cut-off label oszlopban

        QPen dottedPen(Qt::black, 1, Qt::DotLine);
        painter.setPen(dottedPen);

        painter.drawLine(
            QPointF(xCol4 + glueMargin, cellTop),
            QPointF(xCol4 + glueMargin, cellBottom)
            );

        painter.setPen(QPen(Qt::black, 0.75));   // visszaállítjuk

        // következő blokk
        y += blockHeight;

        // oldaltördelés
        if (y + blockHeight > pageRect.bottom()) {
            break;
        }
    }
}



/**/


inline void formatLabelColumnFlow_Pdf(const QVector<LabelModel>& labels,
                                      QPainter& painter,
                                      QPdfWriter& writer,
                                      const QRectF& pageRect,
                                      int columns,
                                      qreal cellHeight)
{
    if (labels.isEmpty() || columns <= 0)
        return;

    const qreal glueMargin = 90.0; // ~0.5 cm
    const qreal barcodeHeight0 = 80.0;   // kétszer magasabb
    const qreal gap = 8.0;   // kb. 2–3 mm
    const qreal barcodeMargin = 80.0;

    QFontMetrics fm(painter.font());
    const qreal lineHeight = fm.height() + 2.0;

    // oszlopszélesség – biztonsági margóval
    const qreal colWidth = (pageRect.width() / columns)-60;

    // emoji panel fix szélessége
    const qreal emojiPanelWidth = cellHeight/2;

    //qreal leftOffset = glueMargin + emojiPanelWidth;

    // belső padding
    const qreal pad = 4.0;

    // hány karakter fér a tartalmi panelbe?
    auto maxCharsForCell = [&](qreal w) -> int {
        qreal avg = fm.horizontalAdvance("W");
        if (avg <= 0) avg = 8.0;
        qreal inner = w - pad * 2;
        int a = qMax(4, int(inner / avg));
        return a;
    };

    const int cellWidthChars = maxCharsForCell(colWidth - emojiPanelWidth);

    struct Cell {
        QString emoji1;            // prio
        QString emoji2; //group icon
        QVector<QString> lines;   // tartalom
        QString barcode;
    };

    QVector<Cell> cells;
    cells.reserve(labels.size());

    // 1) LabelModel → sorokra tördelve
    for (const auto& lm : labels) {
        //        zInfo("labels_tostring:"+lm.toString());
        //        for(const auto& p:lm.parts){
        //            zInfo("_label_part:"+p.text);
        //        }
        QVector<QString> lines =
            buildLabelCellLines(lm.parts, cellWidthChars);
        //QString emoji = "AB";//lm.priorityIcon + lm.groupIcon;
        cells.push_back({ lm.priorityIcon,  lm.groupIcon , lines, lm.barcode });
    }

    // 2) oszlopfolytonos tördelés
    int colIndex = 0;
    int columnNumberLabel = 1;

    qreal x = pageRect.left();
    qreal y = pageRect.top();

    // vastagabb keret
    QPen oldPen = painter.pen();
    painter.setPen(QPen(Qt::black, 1.2));

    auto drawColumnHeader = [&](int num) {
        qreal headerHeight = cellHeight * 0.5;
        QRectF rect(x, y, colWidth, headerHeight);
        painter.drawRect(rect);
        painter.drawText(rect, Qt::AlignCenter, QString::number(num));
        y += headerHeight;
    };

    drawColumnHeader(columnNumberLabel);

    for (int i = 0; i < cells.size(); ++i) {

        const auto& cell = cells[i];

        // új oszlop / új lap
        if (y + cellHeight > pageRect.bottom()) {
            colIndex++;
            columnNumberLabel++;

            if (colIndex >= columns) {
                writer.newPage();
                colIndex = 0;
                //columnNumberLabel = 1;
            }

            x = pageRect.left() + colIndex * colWidth;
            y = pageRect.top();

            drawColumnHeader(columnNumberLabel);
        }

        // cella kerete
        QRectF rect(x, y, colWidth, cellHeight);
        painter.drawRect(rect);

        qreal dottedX = rect.left() + glueMargin;

        // ragasztócsík széle
        QPen dottedPen(Qt::black, 1, Qt::DotLine);
        painter.setPen(dottedPen);
        painter.drawLine(
            QPointF(dottedX, rect.top()),
            QPointF(dottedX, rect.bottom())
            );
        painter.setPen(oldPen);
        // EMOJI PANEL
        // --- EMOJI PANEL: két függőleges félpanel ---
        qreal halfHeight = cellHeight / 2.0;

        // QRectF emojiRectTop(rect.left(), rect.top(), emojiPanelWidth, halfHeight);
        // QRectF emojiRectBottom(rect.left(), rect.top() + halfHeight, emojiPanelWidth, halfHeight);
        QRectF emojiRectTop(rect.left() + glueMargin, rect.top(), emojiPanelWidth, halfHeight);
        QRectF emojiRectBottom(rect.left() + glueMargin, rect.top() + halfHeight, emojiPanelWidth, halfHeight);

        // keretek
        //painter.drawRect(emojiRectTop);
        //painter.drawRect(emojiRectBottom);

        // két karakter szétválasztása
        QString topChar = cell.emoji1;
        QString bottomChar = cell.emoji2;

        // rajzolás
        // painter.drawText(emojiRectTop, Qt::AlignCenter, topChar);
        // painter.drawText(emojiRectBottom, Qt::AlignCenter, bottomChar);

        // QImage topImg = renderEmoji(topChar, emojiPanelWidth);
        // QImage bottomImg = renderEmoji(bottomChar, emojiPanelWidth);

        // painter.drawImage(emojiRectTop, topImg);
        // painter.drawImage(emojiRectBottom, bottomImg);

        // kisebb emoji + gap
        int emojiSize = emojiPanelWidth * 0.8;      // 60% méret
        int emojiMargin = (emojiPanelWidth - emojiSize) / 2;

        // render
        //QImage topImg = EmojiHelper::renderEmojiStandalone(topChar, emojiSize);
        //QImage bottomImg = EmojiHelper::renderEmojiStandalone(bottomChar, emojiSize);

        QPixmap topPixmap   = EmojiHelper::loadPriorityIcon(topChar, emojiSize);
        QPixmap bottomPixmap = EmojiHelper::loadEmoji(bottomChar, emojiSize);

        QImage topImg   = topPixmap.toImage();
        QImage bottomImg = bottomPixmap.toImage();

        // cél téglalapok (gap-pel)
        QRectF topTarget(
            emojiRectTop.left() + emojiMargin,
            emojiRectTop.top() + emojiMargin,
            emojiSize,
            emojiSize
            );

        QRectF bottomTarget(
            emojiRectBottom.left() + emojiMargin,
            emojiRectBottom.top() + emojiMargin,
            emojiSize,
            emojiSize
            );

        // rajzolás
        if (!topChar.isEmpty())
            painter.drawImage(topTarget, topImg);

        if (!bottomChar.isEmpty())
            painter.drawImage(bottomTarget, bottomImg);


        // TARTALMI PANEL
        bool hasEmoji = !(cell.emoji1.isEmpty() && cell.emoji2.isEmpty());

        // QRectF contentRect(
        //     hasEmoji ? rect.left() + emojiPanelWidth : rect.left(),
        //     rect.top(),
        //     hasEmoji ? rect.width() - emojiPanelWidth : rect.width(),
        //     rect.height()
        //     );
        QRectF contentRect(
            hasEmoji ? rect.left() + glueMargin + emojiPanelWidth : rect.left() + glueMargin,
            rect.top(),
            hasEmoji ? rect.width() - glueMargin - emojiPanelWidth : rect.width() - glueMargin,
            rect.height()
            );


        //painter.drawRect(contentRect);

        // vertikális középre igazítás
        qreal totalTextHeight = cell.lines.size() * lineHeight;
        qreal startY = contentRect.top() + (contentRect.height() - totalTextHeight) / 2.0;

        qreal lastTextBottom= 0.0;

        for (int li = 0; li < cell.lines.size(); ++li) {
            QRectF textRect(contentRect.left() + pad,
                            startY + li * lineHeight,
                            contentRect.width() - pad * 2,
                            lineHeight);

            QString txt1 = cell.lines[li];
            painter.drawText(textRect,
                             Qt::AlignLeft | Qt::AlignVCenter,
                             txt1);

            // itt frissítjük a legalsó szövegkoordinátát
            if (textRect.bottom() > lastTextBottom)
                lastTextBottom = textRect.bottom();

            //zInfo("labeltext:"+txt1);
        }


        if (!cell.barcode.isEmpty()) {
            //qreal lastTextBottom = startY + cell.lines.size() * lineHeight;
            qreal barcodeTop = lastTextBottom + gap;   // gap = pl. 6–10 px

            //qreal barcodeBottom = rect.bottom();
            //qreal barcodeHeight = barcodeBottom - barcodeTop;
            qreal cellBottom = contentRect.bottom();
            qreal availableHeight = cellBottom - barcodeTop;
            qreal barcodeHeight = qMin(availableHeight, barcodeHeight0);
            qreal barcodeY = cellBottom - barcodeHeight;
            QRectF bcRect(
                contentRect.left()+barcodeMargin,
                barcodeY,
                contentRect.width()-2*barcodeMargin,
                barcodeHeight
                );
            BarcodePainter::drawCode128(painter, cell.barcode, bcRect);
        }


        y += cellHeight;
    }

    painter.setPen(oldPen);
}

} // end namespace CuttingInstructionUtils

