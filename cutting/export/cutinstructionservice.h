#pragma once

#include "model/cutting/optimizer/optimizermodel.h"
#include <QString>
#include <QUuid>

#include <model/cutting/instruction/cutinstruction.h>
#include "cutting/export/sortmode.h"

class CutInstructionService
{
public:

    enum class ExportMode {
        Standard,
        RodDiagram
    };

    static bool ExportCutPlanSummary(
        const Cutting::Optimizer::OptimizerModel& optimizerModel);

    //void GenerateCutInstructions();
    static bool ExportCutInstructions(
        const QVector<MachineCuts>& _machineCutsList,
        const Cutting::Optimizer::OptimizerModel& optimizerModel,
            ExportMode mode);
    //void ExportCutInstructions_2();
    static bool ExportCutInstructions_Labels(
        const QString& path,
        const QVector<MachineCuts>& machineCutsList,
        QMap<QUuid, QVector<const CutInstruction*>> orderedCuts2);


    static int workflowOrder(MaterialFamily f)
    {
        // Itt majd a saját workflow sorrended:
        // tok → záró → súly → vászon → tengely → láb
        // Példa:
        if (f == MaterialFamily::Tok) return 1;
        if (f == MaterialFamily::TokFed) return 1;

        if (f == MaterialFamily::FelsoSin) return 2;
        if (f == MaterialFamily::Zaro) return 2;
        if (f == MaterialFamily::Palca) return 2;

        if (f == MaterialFamily::Suly) return 3;

        if (f == MaterialFamily::Tengely) return 4;

        if (f == MaterialFamily::Lab) return 5;

        return 999; // fallback
    }

    static void sort(QVector<MachineCuts>* machineCutsList,
                     SortMode mode,
                     const QVector<QString>& prioRefs);


};

