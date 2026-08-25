#include "cutinstructionservice.h"
#include "common/eventlogger.h"
#include "model/cutting/plan/cutplan.h"
#include "service/cutting/instruction/cuttinginstructionutils.h"
#include "service/cutting/summary/cutplansummary.h"
#include <QDir>
#include <service/cutting/instruction/cuttinginstructionutils.h>
#include <service/cutting/summary/cutplansummarybuilder.h>


bool CutInstructionService::ExportCutPlanSummary(
    const Cutting::Optimizer::OptimizerModel& optimizerModel) {

    static const QString errevent = QStringLiteral("❌ Summary export nem hajtható végre. Részletek a logban.");
    static const QString oklog = QStringLiteral("✅ Cut Plan Summary exportálva: %1");

    const auto& plans = optimizerModel.getResult_PlansRef();

    // 1️⃣ Guard: nincs optimalizációs eredmény
    if (plans.isEmpty()) return false;
    /* {
        if (_view)
            _view->ShowWarningDialog(
                "Nincs optimalizációs eredmény.\n"
                "A Summary export nem hajtható végre."
                );
        return;
    }*/

    const auto& leftovers = optimizerModel.getResults_Leftovers();

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zWarning(errevent);
        zEvent("❌ Nincs Cutting Plan fájlnév — a Summary export nem hajtható végre.");
        return false;
    }

    CutPlanSummary summary = CutPlanSummaryBuilder::build(plans, leftovers, baseName, optimizerModel._fitTelemetry);

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);  // ha nincs, létrehozzuk
    QString path = dir + "/" + baseName + "_CutPlanSummary.txt";

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        zWarning(errevent);
        zEvent(QString("❌ Nem sikerült megnyitni a fájlt írásra: %1").arg(path));
        return false;
    }


    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << summary.toText() << "\n";

    file.close();

    zInfo(oklog.arg(QDir::toNativeSeparators(path)));
    zEvent(oklog.arg(QDir::toNativeSeparators(path)));
    return true;
}

bool CutInstructionService::ExportCutInstructions(const QVector<MachineCuts>& machineCutsList,
                                                  const Cutting::Optimizer::OptimizerModel& optimizerModel,
                                                  ExportMode mode)
{

    if (machineCutsList.isEmpty()) {
        // if (_view)
        //     _view->ShowWarningDialog("Nincs legenerált vágási utasítás.\nElőbb futtasd a Generate CutInstructions műveletet.");
        return false;
    }

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zEvent("❌ Nincs Cutting Plan fájlnév — export nem lehetséges.");
        return false;
    }

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);

    QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");
    QMap<QUuid, QVector<const CutInstruction*>> orderedCuts2;

    // --- 1) CutInstructions.txt ---
    {
        QString path;
        if (mode == ExportMode::Standard)
        {
            path = dir + "/" + baseName + "_CutInstructions.txt";
        }
        else if(mode == ExportMode::RodDiagram)
        {
            path = dir + "/" + baseName + "_CutInstructions_Rod.txt";
        }

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            zEvent(L("❌ Nem sikerült megnyitni a %1 fájlt.").arg(path));
            return false;
        }

        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);

        // gépenkénti riport lekérése a for előtt
        const auto& machineReportMap = optimizerModel.getMachineReport();
        const auto& discardedMap = optimizerModel.getDiscardedPieces();

        for (const auto& mc : machineCutsList) {
            const auto& rep = machineReportMap.value(mc.machineHeader.machineId);

            // géphez tartozó FAILED darabok kigyűjtése
            QVector<DiscardedPiece> failedList;
            for (const auto& dp : discardedMap) {
                if (dp.machineId == rep.machineId)
                    failedList.append(dp);
            }

            CuttingInstructionUtils::MachineCutsEvent_Result m;

            if (mode == ExportMode::Standard)
            {
                m = CuttingInstructionUtils::formatMachineCutsEvent(
                    mc, rep, failedList, baseName, SettingsManager::printedLineWidth);
            }
            else if(mode == ExportMode::RodDiagram)
            {
                m = CuttingInstructionUtils::formatMachineCutsEvent_2(
                    mc, rep, failedList, baseName, SettingsManager::printedLineWidth);
            }

            orderedCuts2.insert(mc.machineHeader.machineId, m.orderedCuts);

            out << m.planTxt << "\n\n";
        }

        zEvent(QString("📄 CutInstructions exportálva: %1").arg(path));
    }

    // --- 3) LabelTable PDF ---
    {
        QString path = dir + "/" + baseName + "_CutInstructions_Labels.pdf";

        ExportCutInstructions_Labels(path, machineCutsList, orderedCuts2);
    }

    return true;
}

bool CutInstructionService::ExportCutInstructions_Labels(const QString& path,
                                                         const QVector<MachineCuts>& machineCutsList,
                                                         QMap<QUuid, QVector<const CutInstruction*>> orderedCuts2)
{
    //QString path = dir + "/" + baseName + "_CutInstructions_Labels.pdf";

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt.");
        return false;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    const int cols = 2;
    const qreal cellHeight = 300.0; // nagy, jól olvasható címke

    // MONOSPACED FONT – kötelező a TXT‑s spacinghez
    QFont font("Noto Sans Mono", 11);//Noto Sans Mono
    //QFont font("Courier New", 11);
    painter.setFont(font);

    bool firstPage = true;

    for (const auto& mc : machineCutsList)
    {
        if (!firstPage)
            writer.newPage();   // új lap csak a második géptől

        firstPage = false;

        // QVector<LabelModel> labels =
        //     CuttingInstructionUtils::collectLabelModelsFromMachineCuts(mc);
        QVector<LabelModel> labels =
            CuttingInstructionUtils::collectLabelModelsFromMachineCuts_2(mc.leftoverInfo, orderedCuts2.value(mc.machineHeader.machineId));


        CuttingInstructionUtils::formatLabelColumnFlow_Pdf(
            labels,
            painter,
            writer,
            pageRect,
            cols,
            cellHeight
            );
    }

    painter.end();
    zEvent(QString("🏷️ LabelTable PDF exportálva: %1").arg(path));

    return true;
}

void CutInstructionService::sort(QVector<MachineCuts>* machineCutsList,
                                 SortMode mode,
                                 const QVector<QString>& prioRefs)
{
    if (!machineCutsList)
        return;

    for (auto& mc : *machineCutsList)
    {
        auto& list = mc.cutInstructions;

        std::stable_sort(list.begin(), list.end(),
                         [&](const CutInstruction& a, const CutInstruction& b)
                         {
                             // 1️⃣ PRIORITÁS — mindig előre
                             bool aPrio = prioRefs.contains(a.externalReference);
                             bool bPrio = prioRefs.contains(b.externalReference);

                             if (aPrio != bPrio)
                                 return aPrio;   // aPrio=true → előre kerül

                             // 2️⃣ PRIORITÁSON BELÜL: workflow rendezés + prio sorrend
                             if (aPrio && bPrio)
                             {
                                 auto* mat_a = MaterialRegistry::instance().findById(a.materialId);
                                 auto* mat_b = MaterialRegistry::instance().findById(b.materialId);

                                 if (mat_a && mat_b)
                                 {
                                     int wa = workflowOrder(mat_a->family);
                                     int wb = workflowOrder(mat_b->family);

                                     if (wa != wb)
                                         return wa < wb;   // 🔥 workflow sorrend prio-n belül
                                 }

                                 // 🔥 ha workflow kategória azonos → prioRefs sorrend dönt
                                 int ia = prioRefs.indexOf(a.externalReference);
                                 int ib = prioRefs.indexOf(b.externalReference);
                                 return ia < ib;
                             }


                             // 3️⃣ MARADÉK: a SortMode szerinti rendezés
                             switch (mode)
                             {
                             case SortMode::BySize:
                                 return a.cutSize_mm > b.cutSize_mm;

                             case SortMode::ByMaterial:
                                 return a.materialId.toString() < b.materialId.toString();

                             case SortMode::ByWorkflow:{
                                 auto * mat_a = MaterialRegistry::instance().findById(a.materialId);
                                 auto * mat_b = MaterialRegistry::instance().findById(b.materialId);
                                 if(mat_a && mat_b){
                                    return workflowOrder(mat_a->family)
                                            < workflowOrder(mat_b->family);
                                 } else{
                                     return false;
                                 }
                             }
                             }

                             return false;
                         });
    }
}

bool CutInstructionService::ExportLeftoverLabels(const QString& path,
                                                 const QVector<LabelModel>& labels)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        zEvent("❌ Nem sikerült megnyitni a PDF fájlt (leftover labels).");
        return false;
    }

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());

    const int cols = 2;
    const qreal cellHeight = 300.0;

    QFont font("Noto Sans Mono", 11);
    painter.setFont(font);

    CuttingInstructionUtils::formatLabelColumnFlow_Pdf(
        labels,
        painter,
        writer,
        pageRect,
        cols,
        cellHeight
        );

    painter.end();
    zEvent(QString("🏷️ Leftover Label PDF exportálva: %1").arg(path));

    return true;
}
