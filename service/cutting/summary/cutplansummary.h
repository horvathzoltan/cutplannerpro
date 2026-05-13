#pragma once
#include "cutplan_input_summary.h"
#include "cutplan_output_summary.h"

#include <QDate>


// A CutPlanSummary NEM gépenkénti — ez GLOBÁLIS összefoglaló
struct CutPlanSummary {
    CutPlanInputSummary  input;   // vágás előtti állapot
    CutPlanOutputSummary output;  // vágás utáni állapot

    QString planIdStr;

    QString toText() const {
        QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm");

        QStringList lines;

        lines << QString("📥 Gyártási összefoglaló (globális) - CutPlan: %1").arg(planIdStr);
        lines << QString("📅 Dátum: %1").arg(dateStr);
        lines <<  "────────────────────────────────";
        lines << input.toText();
        lines << "────────────────────────────────";
        lines << output.toText();

        return lines.join("\n");
    }
};
