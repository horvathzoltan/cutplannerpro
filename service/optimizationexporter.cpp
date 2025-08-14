#include "optimizationexporter.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

void OptimizationExporter::exportPlansToCSV(const QVector<Cutting::Plan::CutPlan>& plans, const QString& folderPath)
{
    // 📁 Export mappa előkészítése
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString path = folderPath.isEmpty() ? "exports/" : folderPath;
    QDir().mkpath(path);

    // 📝 Fájl megnyitása
    QFile file(path + QString("/optimized_plans_%1.csv").arg(timestamp));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 🧱 Fejléc: új oszlop → Segments
    out << "PlanId,RodNumber,Barcode,MaterialId,Cuts,KerfTotal,Waste,Source,Segments\n";

    // 🔁 Minden vágási terv kiírása
    for (const auto& plan : plans) {
        QStringList cutLabels;
        for (const Cutting::Piece::PieceWithMaterial& pwm : plan.cuts)
            cutLabels.append(pwm.info.displayText()); // pl. "Kovács BT • MEGR-4022 • 1800 mm"

        QStringList segmentLabels;
        for (const Cutting::Segment::SegmentModel& s : plan.segments)
            segmentLabels.append(s.toLabelString());

        out << plan.planId.toString() << ","
            << plan.rodNumber << ","
            << "\"" << plan.rodId << "\","
            << plan.materialId.toString() << ","
            << "\"" << cutLabels.join(" | ") << "\","
            << plan.kerfTotal << ","
            << plan.waste << ","
            << (plan.source == Cutting::Plan::Source::Reusable ? "Reusable" : "Stock") << ","
            << "\"" << segmentLabels.join(" ") << "\"\n";
    }

    file.close();
}


void OptimizationExporter::exportPlansAsWorkSheetTXT(const QVector<Cutting::Plan::CutPlan>& plans, const QString& folderPath)
{
    // 📁 Export mappa létrehozása
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString path = folderPath.isEmpty() ? "exports/" : folderPath;
    QDir().mkpath(path);

    QFile file(path + QString("/cutplan_worksheet_%1.txt").arg(timestamp));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 📄 Fejléc
    out << "🔧 MUNKALAP — Vágási tervek\n";
    out << QString("Dátum: %1\n").arg(QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm"));
    out << QString("Összes terv: %1\n\n").arg(plans.size());

    // 🔁 Minden CutPlan részletezése
    for (const auto& plan : plans) {
        out << QString("Terv #%1 — PlanId: %2\n").arg(plan.rodNumber).arg(plan.planId.toString());
        out << QString("Anyag Barcode: %1\n").arg(plan.rodId);
        out << QString("Forrás: %1\n").arg(plan.source == Cutting::Plan::Source::Reusable ? "REUSABLE" : "STOCK");
        //out << QString("Darabolások: %1\n").arg(plan.cuts.isEmpty() ? "-" : plan.cutsAsString() + " mm");

        out << QString("Darabolások:\n");

        if (plan.cuts.isEmpty()) {
            out << "-\n";
        } else {
            for (int i = 0; i < plan.cuts.size(); ++i) {
                const auto& pwm = plan.cuts[i];
                out << QString("  %1. %2\n").arg(i + 1).arg(pwm.info.displayText());
            }
        }

        out << QString("Kerf összesen: %1 mm\n").arg(plan.kerfTotal);
        out << QString("Hulladék: %1 mm\n").arg(plan.waste);

        // 🔍 Újdonság: szakaszlista (darabok + kerf-ek + hulladék)
        QStringList segmentLabels;
        for (const Cutting::Segment::SegmentModel& s : plan.segments)
            segmentLabels.append(s.toLabelString());
        out << QString("Szakaszok: %1\n").arg(segmentLabels.join(" "));

        out << QString("Státusz: %1\n").arg(static_cast<int>(plan.status));
        out << "-------------------------------------------\n";
    }

    file.close();
}

