#pragma once

#include "../../../model/cutting/instruction/cutinstruction.h"
#include "common/texthelper.h"

#include <materials/model/material_master.h>

#include <materials/registry/material_registry.h>

#include <model/registries/cuttingplanrequestregistry.h>

#include <QDateTime>
#include <QSet>

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
            refs.append(req->externalReference);
        }
    }

    //std::sort(refs.begin(), refs.end());
    // QVector<QString> refBlocks;
    // int i = 0;
    // while (i < refs.size()) {
    //     int start = refs[i];
    //     int end = start;
    //     int j = i + 1;
    //     while (j < refs.size() && refs[j] == end + 1) {
    //         end = refs[j];
    //         j++;
    //     }
    //     if (start == end)
    //         refBlocks.append(QString("%1.").arg(start));
    //     else
    //         refBlocks.append(QString("%1–%2.").arg(start).arg(end));
    //     i = j;
    // }
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

    int idx = 0;

    for (const auto& ci : mc.cutInstructions) {

        bool rodChanged = (ci.rodId != prevRod);

        if (rodChanged && !prevRod.isEmpty())
            lines << "────────────────────────────────";

        prevRod = ci.rodId;

        QString rodLabel = rodChanged
                               ? QString("%1 □").arg(ci.rodId)
                               : ci.rodId + "   ";

        QString step = QString("%1.").arg(ci.globalStepId, width, 10, QLatin1Char(' '));

        QString icon = ci.isManualCut ? "📏" : "✂️";

        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(ci.materialId);

        QString materialLabel = mat
                                    ? QString("%1:%2").arg(mat->name).arg(ci.barcode)
                                    : QString("Material:%1").arg(ci.materialId.toString(QUuid::WithoutBraces));

        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);

        QString pieceLabel = req
                                 ? QString("%1. %2").arg(req->externalReference).arg(req->ownerName)
                                 : QString("req:%1").arg(ci.requestId.toString(QUuid::WithoutBraces));

        // ➋ méret jobbra igazítva, fix szélességgel
        QString sizeStr = sizeStrings[idx++];
        QString sizePadded = QString("%1").arg(sizeStr, maxSizeLen, QLatin1Char(' '));

        // ➌ pieceLabel vágása printedLW alapján
        int fixedLen =
            step.length() + 1 +
            rodLabel.length() + 3 +
            materialLabel.length() + 3 +
            icon.length() + 1 +
            sizePadded.length() + 5; // " mm □ | "

        int maxPieceLen = printedLW - fixedLen - 2; // " □"
        if (maxPieceLen < 5) maxPieceLen = 5;

        QString piece = pieceLabel;
        if (piece.length() > maxPieceLen)
            piece = piece.left(maxPieceLen - 1) + "…";

        lines << QString("%1 %2 | %3 | %4 %5 mm □ | %6 □")
                     .arg(step)
                     .arg(rodLabel)
                     .arg(materialLabel)
                     .arg(icon)
                     .arg(sizePadded)
                     .arg(piece);
    }

    return lines.join("\n");
}





struct LabelPart {
    QString text;
    bool trimmable = false;
};

struct LabelModel {
    QVector<LabelPart> parts;

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



inline QVector<LabelModel> collectLabelModelsFromMachineCuts(const MachineCuts& mc)
{
    QVector<LabelModel> out;
    QSet<QString> rodSeen;

    for (const auto& ci : mc.cutInstructions) {

        // 1) Rúd címke
        if (!rodSeen.contains(ci.rodId)) {
            rodSeen.insert(ci.rodId);

            LabelModel rod;
            rod.parts.append({ ci.rodId, false });
            out.append(rod);
        }

        // 2) Darab címke
        auto req = CuttingPlanRequestRegistry::instance().findById(ci.requestId);

        QString ext = req ? req->externalReference : "req";
        QString owner = req ? req->ownerName : ci.requestId.toString(QUuid::WithoutBraces);
        QString sizeStr = QString("%1 mm").arg(QString::number(ci.cutSize_mm, 'f', 0));

        LabelModel lm;
        lm.parts.append({ ext+'.', false });
        lm.parts.append({ " ", false });
        lm.parts.append({ owner, true });      // csak ez trimmelhető
        lm.parts.append({ " | ", false });
        lm.parts.append({ sizeStr, false });

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

inline QString formatLabelTable4(const QVector<LabelModel>& models,
                                 int pageWidth,
                                 int cols,
                                 int cellHeight)
{
    int cellWidth = (pageWidth - (cols + 1)) / cols;
    if (cellWidth < 10) cellWidth = 10;

    QStringList out;

    int total = models.size();
    int rows = (total + cols - 1) / cols;

    int idx = 0;

    for (int r = 0; r < rows; ++r) {

        // felső vagy köztes keret
        if (r == 0) {
            QString top = "┌";
            for (int c = 0; c < cols; ++c) {
                top += makeHLine(cellWidth);
                top += (c == cols - 1 ? "┐" : "┬");
            }
            out << top;
        } else {
            out << makeMiddleBorder(cols, cellWidth);
        }

        int contentRow = (cellHeight == 1 ? 0 : 1);

        for (int h = 0; h < cellHeight; ++h) {

            QString row = "│";

            for (int c = 0; c < cols; ++c) {

                QString text;

                if (idx < total && h == contentRow) {
                    LabelModel lm = models[idx];
                    trimLabelToWidth(lm, cellWidth);
                    text = lm.toString();
                }

                int pad = (cellWidth - text.length()) / 2;
                if (pad < 0) pad = 0;

                row += makeSpaces(pad)
                       + text
                       + makeSpaces(cellWidth - pad - text.length())
                       + "│";

                if (h == contentRow) idx++;
            }

            out << row;
        }

        if (r == rows - 1) {
            QString bottom = "└";
            for (int c = 0; c < cols; ++c) {
                bottom += makeHLine(cellWidth);
                bottom += (c == cols - 1 ? "┘" : "┴");
            }
            out << bottom;
        }
    }

    return out.join("\n");
}


} // end namespace CuttingInstructionUtils

