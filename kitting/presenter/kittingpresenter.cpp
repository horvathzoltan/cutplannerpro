#include "kittingpresenter.h"
#include "product/utils/material_role_utils.h"
#include "view/MainWindow.h"

#include "common/eventlogger.h"
#include "common/logger.h"

#include <model/registries/cuttingplanrequestregistry.h>

#include <QDir>
#include <QFileInfo>

#include <kitting/kittingengine.h>

KittingPresenter::KittingPresenter(MainWindow* view, QObject* parent)
    : QObject(parent), _view(view){}

void KittingPresenter::GenerateKittingInstructions()
{
    auto cp = _view->cuttingPresenter();
    auto opmod = cp->optimizerModel();

    const QVector<Cutting::Plan::CutPlan> &cutPlans = opmod->getResult_PlansRef();
    if (cutPlans.isEmpty()) {
        ValidationResult r;
        r.errors << "Nincs optimalizációs eredmény.\nElőbb futtasd az Optimize műveletet.";
        _view->ShowWarningDialog(r);
        return;
    }

    _instructions.clear();
    QHash<QUuid,int> requestRoleCounters;
    int globalStep = 1;

    for (const auto& plan : cutPlans) {

        for (int i = 0; i < plan._segments.size(); ++i) {

            const auto& seg = plan._segments.segment(i);
            if (!seg.isPiece()) continue;

            auto pwm = plan.getPieceMaterialBy_pieceId(seg._pieceId);
            auto* request = CuttingPlanRequestRegistry::instance().findById(seg._requestId);
            const MaterialMaster* mat = MaterialRegistry::instance().findById(pwm.materialId);

            // --- 1) Vágott darab kitting utasítása ---
            // KittingInstruction ki;
            // ki.globalStepId = globalStep++;
            // ki.requestId    = seg._requestId;
            // ki.role         = MaterialRoleUtils::makeRole(*request, mat);
            // ki.materialId   = plan.materialId;
            // ki.externalReference = seg.externalReference;
            // ki.attributes   = pwm.attributes;
            // ki.roleCounter  = ++requestRoleCounters[seg._requestId];

            // _instructions.push_back(ki);

            // --- 2) BOM + CutPlan + Request → további kitting utasítások ---
            auto extra = KittingEngine::expand(*request, pwm, plan);
            for (auto& e : extra) {
                e.globalStepId = globalStep++;
                _instructions.push_back(e);
            }
        }
    }
}

QMap<QString, QVector<KittingInstruction>>
KittingPresenter::groupByExternalRef(const QVector<KittingInstruction>& list)
{
    QMap<QString, QVector<KittingInstruction>> grouped;

    for (const auto& ki : list) {
        grouped[ki.externalReference].append(ki);
    }

    return grouped;
}

// QMap<QString,int> KittingPresenter::countByMaterial(const QVector<KittingInstruction>& items)
// {
//     QMap<QString,int> out;

//     for (const auto& ki : items) {
//         const MaterialMaster* mat =
//             MaterialRegistry::instance().findById(ki.materialId);

//         QString key = mat->toReportLabel();
//         out[key] += 1;
//     }

//     return out;
// }

QMap<QString, KittingPresenter::MaterialSummary>
KittingPresenter::countByMaterial(const QVector<KittingInstruction>& items)
{
    QMap<QString, MaterialSummary> out;

    for (const auto& ki : items) {
        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(ki.materialId);

        QString key = mat->toReportLabel();

        // ha még nincs ilyen anyag, inicializáljuk
        if (!out.contains(key)) {
            out[key] = { ki.quantity, ki.unit };
        }
        else {
            // csak akkor adunk össze, ha ugyanaz a mértékegység
            if (out[key].unit == ki.unit)
                out[key].quantity += ki.quantity;
            else {
                // külön egység → külön sor kellene
                // de ez ritka, és általában nem fordul elő
            }
        }
    }

    return out;
}

void KittingPresenter::ExportKittingInstructions()
{
    if (_instructions.isEmpty()) {
        ValidationResult r;
        r.errors << "Nincs legenerált kitting utasítás.\n"
                 << "Előbb futtasd a Generate KittingInstructions műveletet.";
        if (_view)            
            _view->ShowWarningDialog(r);
        return;
    }

    QString fileName = SettingsManager::instance().cuttingPlanFileName();
    QFileInfo fi(fileName);
    QString baseName = fi.completeBaseName();

    if (baseName.isEmpty()) {
        zEvent("❌ Nincs Kitting Plan fájlnév — export nem lehetséges.");
        return;
    }

    QString dir = fi.absolutePath() + "/_reports";
    QDir().mkpath(dir);

    QString path = dir + "/" + baseName + "_KittingInstructions.txt";
    QFile f(path);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        zEvent("❌ Nem sikerült megnyitni a KittingInstructions fájlt.");
        return;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out << "Kitting Instructions (Aggregált nézet)\n";
    out << "Generated: "
        << QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm")
        << "\n\n";

    // 🔥 1) Csoportosítás tételszám (externalReference) szerint
    auto grouped = groupByExternalRef(_instructions);

    // 🔥 2) Tételszám-blokkok kiírása
    for (auto it = grouped.begin(); it != grouped.end(); ++it)
    {
        QString tetelszam = it.key();
        const auto& items = it.value();

        // Request metaadatok
        const Cutting::Plan::Request* req =
            CuttingPlanRequestRegistry::instance().getFirstRequest(tetelszam);
        QString separator = QString(SettingsManager::printedLineWidth, '=');
        out << separator << "\n";
        out <<  req->toString2() << "\n";
        out << "kitting elemek: " << items.size() << " db\n";
        out << separator << "\n";

        // 🔥 3) Anyagonkénti összesítés
        auto counts = countByMaterial(items);

        // 1) leghosszabb anyagnév hossza
        qsizetype maxLen = 0;
        for (auto it2 = counts.begin(); it2 != counts.end(); ++it2) {
            maxLen = std::max(maxLen, it2.key().length());
        }

        // 2) kiírás igazítással
        for (auto it2 = counts.begin(); it2 != counts.end(); ++it2) {

            QString name = it2.key();
            double qty = it2.value().quantity;
            QString unit = it2.value().unit;

            out << "  "
                << name.leftJustified(maxLen + 2, ' ')
                << QString::number(qty).rightJustified(6, ' ')
                << " " << unit << "\n";
        }

        out << "\n";
    }

    zEvent(QString("📄 KittingInstructions exportálva: %1").arg(path));
}


