#pragma once
#include <QDateTime>
#include <QPainter>
#include <QSet>

#include "../../../model/cutting/instruction/cutinstruction.h"
#include "common/texthelper.h"
#include <materials/model/material_master.h>
#include <materials/registry/material_registry.h>
#include <model/registries/cuttingmachineregistry.h>
#include <model/registries/cuttingplanrequestregistry.h>
#include <model/registries/stockregistry.h>
#include <model/cutting/cuttingmachine.h>
#include <model/storageaudit/storageauditrow.h>
#include <service/storageaudit/storageauditservice.h>
#include <common/identifierutils.h>
#include <common/settingsmanager.h>


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
inline QString formatMachineCutsEvent(const MachineCuts& mc, const QString& planIdStr, const int printedLW)
{
    QStringList lines;
    //QString planId = mc.machineHeader.planId.toString();
    QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

    int inputCount = 0;
    QStringList refs;
    for (const auto& ci : mc.cutInstructions) {
        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);
        if (req) {
            inputCount += 1;//req->quantity;
            //bool ok = false;
            //int v = req->externalReference.toInt(&ok);
            //if (ok) refs.append(v);
            refs.append(ci.externalReference);
        }
    }

    QString compressed = TextHelper::compressRanges_String(refs);

    int outputCount = 0;
    for (const auto& ci : mc.cutInstructions)
        outputCount++;

    int diff = outputCount - inputCount;

    lines << QString("🧾 Vágási utasítások (gépenkénti)");
    lines << QString("CutPlan: %1").arg(planIdStr);
    lines << QString("📅 Dátum: %1").arg(dateStr);
    lines << QString("🪚 Gép: %1").arg(mc.machineHeader.machineName);
    lines << "────────────────────────────────";
    lines << buildMaterialStockReportForMachine_AUDIT(mc);
    lines << "────────────────────────────────";

    // --- új: szálak és hullók összesítése ---

    QSet<QString> usedLeftoverRods;

    for (const auto& ci : mc.cutInstructions) {
        if (ci.source == Cutting::Plan::Source::Reusable)
            usedLeftoverRods.insert(ci.barcode);//.insert(ci.rodId+ " → " + ci.barcode);
    }

    QStringList scrapList = usedLeftoverRods.values();
    // scrapList.sort();

    if (!scrapList.isEmpty()) {
        lines << "♻️ Érintett hullók:";
        lines << QString("  • %1 db (%2)")
                     .arg(scrapList.size())
                     .arg(scrapList.join(", "));
        lines << "────────────────────────────────";
    }


    //

    lines << "📥 Gyártási input:";
    lines << QString("  • Kért darabszám: %1 db").arg(inputCount);
    lines << QString("  • Kért tételszámok: %1").arg(compressed);
    lines << "📤 Gyártási output:";
    lines << QString("  • Levágott darabok: %1 db").arg(outputCount);
    lines << QString("  • Eltérés: %1 db").arg(diff);
    lines << "────────────────────────────────";

    QString prevRod;

    int maxStep = mc.cutInstructions.isEmpty()
                      ? 0
                      : mc.cutInstructions.last().globalStepId;

    int width = qMax(3, QString::number(maxStep).length());

    // ➊ max méret-szélesség kiszámítása
    int maxSizeLen = 0;
    QVector<QString> sizeStrings;
    sizeStrings.reserve(mc.cutInstructions.size());

    for (const auto& ci : mc.cutInstructions) {
        QString s = QString::number(ci.cutSize_mm, 'f', 1); // pl. "1145.0"
        sizeStrings.append(s);
        if (s.length() > maxSizeLen)
            maxSizeLen = s.length();
    }

    // ismétrődő méret kiszámolás
    QHash<QString, int> sizeCount;
    for (const auto& ci : mc.cutInstructions) {
        QString s = QString::number(ci.cutSize_mm, 'f', 1);
        sizeCount[s] += 1;
    }

    int idx = 0;

    // --- 1. FÁZIS: OSZLOPOK ELŐGYŰJTÉSE ---
    QVector<QString> colStep;
    QVector<QString> colRod;
    QVector<QString> colMat;
    QVector<QString> colIcon;
    QVector<QString> colSize;
    QVector<QString> colPiece;

    QString prevRodTmp;

    int idxTmp = 0;
    for (const auto& ci : mc.cutInstructions) {

        // QString rodLabelTmp = (ci.rodId != prevRodTmp)
        // ? QString("%1 □").arg(ci.rodId)
        // : ci.rodId + "  ";

        QString rodIdOrBarcodeTmp = (ci.source == Cutting::Plan::Source::Reusable)
                                        ? ci.barcode
                                        : ci.rodId;

        QString rodLabelTmp = (rodIdOrBarcodeTmp != prevRodTmp)
                                  ? QString("%1 □").arg(rodIdOrBarcodeTmp)
                                  : rodIdOrBarcodeTmp + "  ";


        prevRodTmp = ci.rodId;

        QString stepTmp = QString("%1.").arg(ci.globalStepId, width, 10, QLatin1Char(' '));
        QString iconTmp = ci.isManualCut ? "📏" : "✂️";

        const MaterialMaster* matTmp =
            MaterialRegistry::instance().findById(ci.materialId);

        QString materialLabelTmp = matTmp
                                       ? QString("%1").arg(matTmp->name)
                                       : QString("Material:%1").arg(ci.materialId.toString(QUuid::WithoutBraces));

        QString sizeStrTmp = sizeStrings[idxTmp++];
        QString sizeFullTmp = QString("%1 mm □").arg(sizeStrTmp);

        auto reqTmp = CuttingPlanRequestRegistry::instance().findById(ci.requestId);
        QString pieceLabelTmp = reqTmp
                                    ? QString("%1. %2").arg(ci.externalReference).arg(reqTmp->ownerName)
                                    : QString("req:%1").arg(ci.requestId.toString(QUuid::WithoutBraces));

        colStep.append(stepTmp);
        colRod.append(rodLabelTmp);
        colMat.append(materialLabelTmp);
        colIcon.append(iconTmp);
        colSize.append(sizeFullTmp);
        colPiece.append(pieceLabelTmp);
    }

    // oszlopszélességek kiszámítása
    auto maxWidth = [](const QVector<QString>& v){
        int m = 0;
        for (const auto& s : v) m = qMax(m, s.length());
        return m;
    };

    int wStep = maxWidth(colStep);
    int wRod  = maxWidth(colRod);
    int wMat  = maxWidth(colMat);
    int wIcon = maxWidth(colIcon);
    int wSize = maxWidth(colSize);

    QString prevSizeStr;
    int repeatCount = 1;
    bool firstOfBlock = true;


    // --- 2. FÁZIS: RENDERELÉS DINAMIKUS OSZLOPSZÉLESSÉGGEL ---
    prevRod.clear();
    idx = 0;

    for (const auto& ci : mc.cutInstructions) {

        bool rodChanged = (ci.rodId != prevRod);
        if (rodChanged && !prevRod.isEmpty())
            lines << "──────────────";
        prevRod = ci.rodId;

        // QString rodLabel = (rodChanged)
        //                        ? QString("%1 □").arg(ci.rodId)
        //                        : ci.rodId + "  ";
        QString rodIdOrBarcode = (ci.source == Cutting::Plan::Source::Reusable)
                                     ? ci.barcode
                                     : ci.rodId;

        QString rodLabel = (rodChanged)
                               ? QString("%1 □").arg(rodIdOrBarcode)
                               : rodIdOrBarcode + "  ";

        QString step = QString("%1.").arg(ci.globalStepId, width, 10, QLatin1Char(' '));
        QString icon = ci.isManualCut ? "📏" : "✂️";

        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(ci.materialId);

        QString materialLabel = mat
                                    ? QString("%1").arg(mat->barcode)
                                    : QString("Material:%1").arg(ci.materialId.toString(QUuid::WithoutBraces));

        QString sizeStr = sizeStrings[idx++];
        QString sizeFull = QString("%1 mm □").arg(sizeStr);

        // ismétlődés detektálása
        bool sameAsPrev = (sizeStr == prevSizeStr);

        if (sameAsPrev) {
            repeatCount++;
            firstOfBlock = false;
        } else {
            // ha új méret jön, az előző blokkot lezárjuk
            repeatCount = 1;
            firstOfBlock = true;
        }

        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);
        QString pieceLabel = req
                                 ? QString("%1. %2").arg(ci.externalReference).arg(req->ownerName)
                                 : QString("req:%1").arg(ci.requestId.toString(QUuid::WithoutBraces));

        // --- oszlopok paddinggel ---
        QString stepP = step.leftJustified(wStep, ' ');
        QString rodP  = rodLabel.leftJustified(wRod, ' ');
        QString matP  = materialLabel.leftJustified(wMat, ' ');
        QString iconP = icon.leftJustified(wIcon, ' ');
        QString sizeP = sizeFull.rightJustified(wSize, ' ');

        // --- pieceLabel vágása printedLW alapján ---
        int fixedLen = wStep + 1 + wRod + 3 + wMat + 3 + wIcon + 1 + wSize + 5;
        int maxPieceLen = printedLW - fixedLen;
        if (maxPieceLen < 5) maxPieceLen = 5;

        QString piece = pieceLabel;
        if (piece.length() > maxPieceLen)
            piece = piece.left(maxPieceLen - 1) + "…";

        // bool isRepeatedSize = (sizeCount.value(sizeStr, 0) > 1);
        // QString sepAfterSize = isRepeatedSize ? " ║ " : " | ";

        // QString multiplier = "";
        // if (isRepeatedSize && firstOfBlock) {
        //     multiplier = QString("  ×%1").arg(sizeCount[sizeStr]);
        // }

        int count = sizeCount.value(sizeStr, 0);
        bool isRepeated = (count > 1);

        QString sepAfterSize;
        // if (!isRepeated) {
        //     sepAfterSize = " | ";
        // } else if (firstOfBlock) {
        //     sepAfterSize = "  ╽ "; //╽
        // } else if (repeatCount == count) {
        //     sepAfterSize = " ╿ "; //╿
        // } else {
        //     sepAfterSize = " ┃ ";
        // }

        if (!isRepeated) {
            sepAfterSize = " | ";
        } else if (firstOfBlock) {
            sepAfterSize = " ╖ "; //╽
        } else if (repeatCount == count) {
            sepAfterSize = " ╜ "; //╿
        } else {
            sepAfterSize = " ║ ";//┃
        }

        QString multiplier = "";
        if (isRepeated && firstOfBlock) {
            multiplier = QString("  ×%1").arg(count);
        }

        // --- végső sor ---
        lines << QString("%1 %2 | %3 | %4 %5%6%7%8")
                     .arg(stepP)
                     .arg(rodP)
                     .arg(matP)
                     .arg(iconP)
                     .arg(sizeP)
                     .arg(sepAfterSize)
                     .arg(piece+" □")
                     .arg(multiplier);

        prevSizeStr = sizeStr;
    }


    return lines.join("\n");
}





struct LabelPart {
    QString text;

    bool trimmable = false;   // rövidíthető
    bool jumpable = false;    // kiugorhat új sorba
    int targetRow = 0;        // melyik sorba szeretnénk alapból
    Qt::Alignment align = Qt::AlignLeft;  // bal/közép/jobb
};


struct LabelModel {
    QVector<LabelPart> parts;
    QString priorityIcon;   // 🔥💧☁️🪨
    QString groupIcon;      // 🦌🐸🐱… ABC állatok

    QString toString() const {
        QString out;
        for (const auto& p : parts)
            out += p.text;
        return out;
    }

    int length() const {
        int len = 0;
        for (const auto& p : parts)
            len += p.text.length();
        return len;
    }
};

// PRIORITY‑ICON BUCKETING, PRIORITY ICON SCALE
// Lean / Kanban / Toyota Production System egyik alapelve:
// A prioritás vizuális, ikonikus, azonnal felismerhető legyen.

inline QString priorityIconFor(const QDate& dueDate)
{
    if (!dueDate.isValid())
        return "🌞";   // nincs határidő → nincs prio

    QDate today = QDate::currentDate();
    int daysLeft = today.daysTo(dueDate);

    if (daysLeft <= 1)
        return "🔥";   // Tűz – SOS
    if (daysLeft <= 3)
        return "💦";   // Víz – sürgős 💧 🌊 💦
    if (daysLeft <= 6)
        return "💨";  // Levegő – normál 🌥️  💨
    return "⏳";       // Föld – ráér
}

static const QStringList GROUP_ICONS = {
    "🍎", // A - Alma
    "🐸", // B - Béka
    "🐈‍", // C - Macska
    "🦇", // D - Denevér
    "🐭", // E - Egér
    "🍦", // F - Fagyi
    "🦎", // G - Gekkó
    "🌶️", // H - Chili
    "🐛", // I - Kukac
    "🐆", // J - Jaguár
    "❤", // K - szív
    "🦋", // L - Lepke
    "🐿️", // M - Mókus
    "🐰", // N - Nyúl
    "🦁", // O - Oroszlán
    "🐼", // P - Panda
    "🦊", // R - Róka
    "🦔", // S - Süni
    "🐢", // T - Teknős
    "🍇", // V - Szőlő
    "🦓"  // Z - Zebra
};


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
        QString icon = GROUP_ICONS[groupIndex % GROUP_ICONS.size()];

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

            LabelModel rod;
            rod.parts.append({
                ci.rodId,
                false, false,
                0,
                Qt::AlignCenter
            });

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

        //lm.parts.append({ ext + " ", false, false, 0, Qt::AlignLeft });
        lm.parts.append({ prio + group + " " + ext + " ", false, false, 0, Qt::AlignLeft });
        lm.parts.append({ owner,     true,  true,  0, Qt::AlignCenter });

        QString a = "";
        if(ci.subtype != Subtype::None){
            a+= SubtypeUtils::toDisplayText(ci.subtype);
        }

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

    QVector<QStringList> boxes;

    // 1) minden címke külön tartalom (keret nélkül)
    for (auto& m : models)
        boxes << buildLabelCellLines(m.parts, cellWidth);

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
        if (p + 1 < pages.size())
            out << "\f"; // form feed - új oldal

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
    lines << QString("🧾 Leftover felvételi űrlap (manual RSM címkék)");
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

    for (int i = 0; i < rowsPerPage; ++i) {
        QString code = IdentifierUtils::makeManualLeftoverId(next++);

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


inline void formatLabelColumnFlow_Pdf(const QVector<LabelModel>& labels,
                                      QPainter& painter,
                                      const QRectF& pageRect,
                                      int cols,
                                      qreal cellHeight)
{
    if (cols <= 0 || labels.isEmpty())
        return;

    const qreal cellWidth = pageRect.width() / cols;
    const QFontMetrics fm(painter.font());

    auto maxCharsForCell = [&](qreal w) -> int {
        // nagyon egyszerű becslés: átlagos karakter-szélesség
        qreal avg = fm.horizontalAdvance(QStringLiteral("M"));
        if (avg <= 0) avg = 8.0;
        int inner = int(w) - 2; // 1-1 px padding
        return qMax(4, int(inner / avg));
    };

    const int cellWidthChars = maxCharsForCell(cellWidth);

    int col = 0;
    int row = 0;

    auto newPage = [&]() {
        // hívja a hívó: QPdfWriter esetén: writer.newPage();
        // itt csak a sor/col reset:
        col = 0;
        row = 0;
    };

    for (const auto& srcLm : labels) {
        LabelModel lm = srcLm;                 // lokális másolat, hogy trimmelhessünk
        trimLabelToWidth(lm, cellWidthChars);  // meglévő helper

        QVector<QString> lines = buildLabelCellLines(lm.parts, cellWidthChars);

        // ha nem fér el az aktuális oldalon, új oldal
        qreal topY = pageRect.top() + row * cellHeight;
        if (topY + cellHeight > pageRect.bottom()) {
            // itt a hívó felelőssége: writer.newPage();
            painter.translate(0, 0); // semmi, csak jelzés
            newPage();
            topY = pageRect.top();
        }

        qreal leftX = pageRect.left() + col * cellWidth;
        QRectF cellRect(leftX, topY, cellWidth, cellHeight);

        // keret
        painter.drawRect(cellRect);

        // belső tartalom
        const qreal innerLeft   = cellRect.left() + 2;
        const qreal innerTop    = cellRect.top() + 2;
        const qreal innerWidth  = cellRect.width() - 4;
        const qreal lineHeight  = fm.height() + 2;

        for (int i = 0; i < lines.size(); ++i) {
            QRectF textRect(innerLeft,
                            innerTop + i * lineHeight,
                            innerWidth,
                            lineHeight);
            painter.drawText(textRect,
                             Qt::AlignLeft | Qt::AlignVCenter,
                             lines[i]);
        }

        // következő cella
        ++col;
        if (col >= cols) {
            col = 0;
            ++row;
        }
    }
}

} // end namespace CuttingInstructionUtils

