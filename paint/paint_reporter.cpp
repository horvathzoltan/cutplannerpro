#include "paint_calculator.h"
#include "paint_reporter.h"
#include "common/eventlogger.h"
#include "model/cutting/plan/audit/naphalo_profile_postfix.h"

#include <materials/registry/material_registry.h>

#include <model/registries/cuttingplanrequestregistry.h>

#include <settings/settingsmanager.h>

#include <QDir>
#include <QFileInfo>

QString PaintReporter::toText(const PaintPlan& plan)
{
    QStringList out;
    //out << "=== FESTÉSI TERV (TXT) ===";



    //PaintPlan plan = buildPaintPlan();

    // Színek ábécé sorrendben
    QStringList colors = plan.byColor.keys();
    colors.sort(Qt::CaseInsensitive);

    for (const QString& color : colors)
    {
        const auto& colorGroup = plan.byColor[color];

        // --- HA ÜRES A CSOPORT → KIHAGYJUK ---
        if (colorGroup.materials.isEmpty() &&
            colorGroup.sumPofa() == 0 &&
            colorGroup.csavar == 0)
        {
            continue;   // ❗ NEM írjuk ki a színblokkot
        }

        out << "";
        out<< QString("SZÍN: %1").arg(colorGroup.color.toString());
        out<< "──────────────────────────────────";

        // Anyagok ábécé sorrendben
        QList<QUuid> materials = colorGroup.materials.keys();
        std::sort(materials.begin(), materials.end(),
                  [](const QUuid& a, const QUuid& b){
                      return a.toString() < b.toString();
                  });

        for (const QUuid& matId : materials)
        {
            const auto& s = colorGroup.materials[matId];

            bool isCompositeCL = (matId == PaintCalculator::CL_COMPOSITE_ID);

            QString matName;
            QString postfix;
            double keruletCm = 0.0;

            if (isCompositeCL)
            {
                matName = "CipzárosLáb + Lábtakaró [NP-CL/CLT]";
                keruletCm = 27.0;
            }
            else
            {
                const MaterialMaster* mat = MaterialRegistry::instance().findById(matId);
                matName = mat ? mat->toDisplay() : "???";

                QString barcode = mat ? mat->barcode : "???";
                postfix = ProfileUtils::profilePostfixFor(barcode);
            }

            out<<QString("   Anyag: %1").arg(matName);

            if (isCompositeCL)
            {
                out << QString("      Teljes hossz: %1 mm (%2 m × %3 cm)")
                           .arg(s.totalLength_mm)
                           .arg(s.totalLength_mm / 1000.0, 0, 'f', 2)
                           .arg(keruletCm);
            }
            else
            {
                if (!postfix.isEmpty()) {
                    out << QString("      Teljes hossz: %1 mm (%2 m × %3)")
                               .arg(s.totalLength_mm)
                               .arg(s.totalLength_mm / 1000.0, 0, 'f', 2)
                               .arg(postfix);
                } else {
                    out << QString("      Teljes hossz: %1 mm (%2 m)")
                    .arg(s.totalLength_mm)
                        .arg(s.totalLength_mm / 1000.0, 0, 'f', 2);
                }
            }

            out<<QString("      Porfogyás: %1 kg")
                       .arg(s.powderKg, 0, 'f', 2);

            // Request ID lista
            QStringList tetelszamok;
            for (const auto& id : s.requestIds) {
                auto *a = CuttingPlanRequestRegistry::instance().findById(id);
                QString b = a ? a->externalReference : "?";
                QString c = a ? a->dueDate.toString("yyyy-MM-dd") : "?";
                tetelszamok << QString("%1 (%2)").arg(b, c);
            }

            out<<QString("      Tételszámok: %1").arg(tetelszamok.join(", "));
            out<<"";
        }

        // --- TÍPUSONKÉNTI POFÁK ---
        if (colorGroup.sumPofa() > 0)
        {
            out<<"   POFÁK:";
            if(colorGroup.cipzarosPofa>0){
                out<<QString("      Cipzáros: %1").arg(colorGroup.cipzarosPofa);
            }
            if(colorGroup.sinesPofa > 0){
                out<<QString("      Sines:    %1").arg(colorGroup.sinesPofa);
            }
            if(colorGroup.bowdenesPofa > 0){
                out<<QString("      Bowdenes: %1").arg(colorGroup.bowdenesPofa);
            }
            QString postfix1 = ProfileUtils::profilePostfixFor("NP-POF");
            if (!postfix1.isEmpty()) {
                out<<QString("      Összesen: %1 db, %2").arg(colorGroup.sumPofa()).arg(postfix1);
            } else{
                out<<QString("      Összesen: %1 db").arg(colorGroup.sumPofa());
            }
            out<<QString("      Porfogyás: %1 kg")
                       .arg(colorGroup.pofaPowderKg, 0, 'f', 2);

        }
        else
        {
            if (colorGroup.pofaFestheto)
                // ❗ HIBA: nincs pofa, pedig kellene - de csak ha festhető a tok
                out<<"   ⚠️ HIBA: Ehhez a színhez nem számolódott pofa!";
            else
                out<<"   (Ehhez a színhez nem kell pofa)";
        }

        // --- TÍPUSONKÉNTI CSAVAROK ---
        if (colorGroup.csavar > 0)
        {
            out<<"   CSAVAROK:";
            QString postfix2 = ProfileUtils::profilePostfixFor("NP-CSAV");
            if (!postfix2.isEmpty()) {
                out<<QString("      Összesen: %1 db, %2").arg(colorGroup.csavar).arg(postfix2);
            } else{
                out<<QString("      Összesen: %1 db").arg(colorGroup.csavar);
            }
        }
        else
        {
            if (colorGroup.csavarFestheto)
                // ❗ HIBA: nincs csavar, pedig kellene - de csak ha festhető a tokfedél
                out<<"   ⚠️ HIBA: Ehhez a színhez nem számolódott csavar!";
            else
                out<<"   (Ehhez a színhez nem kell csavar)";
        }

        out<<"";
        out<<QString("   Összes porfogyás: %1 kg")
                   .arg(colorGroup.powderKg, 0, 'f', 2);

        out<<"──────────────────────────────────";
    }

    //out<<"=== FESTÉSI TERV VÉGE ===";
    return out.join("\n");
}

void PaintReporter::exportText(const QString& txt)
{

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);

    QString dateStr = QDateTime::currentDateTime().toString("yyyy.MM.dd_HH-mm");
    QString path = QString("%1/paintplan_%2.txt").arg(dir, baseName);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        zEvent("❌ Nem sikerült megnyitni a PaintPlan fájlt.");
        return;
    }


    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out << QString("📄 Festési terv");
    out << QString("CutPlan: %1").arg(baseName);
    out << QString("📅 Dátum: %1").arg(dateStr);
    out << "──────────────────────────────────";


    out << txt;

    zEvent(QString("🎨 Festési terv exportálva: %1").arg(path));
}