#include "kittingpresenter.h"
#include "product/material_role_utils.h"
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

    QVector<Cutting::Plan::CutPlan> &cutPlans = opmod->getResult_PlansRef();
    if (cutPlans.isEmpty()) {
        _view->ShowWarningDialog("Nincs optimalizációs eredmény.\nElőbb futtasd az Optimize műveletet.");
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


void KittingPresenter::ExportKittingInstructions()
{
    if (_instructions.isEmpty()) {
        if (_view)
            _view->ShowWarningDialog(
                "Nincs legenerált kitting utasítás.\n"
                "Előbb futtasd a Generate KittingInstructions műveletet."
                );
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

    out << "Kitting Instructions\n";
    out << "Generated: "
        << QDateTime::currentDateTime().toString("yyyy.MM.dd HH:mm")
        << "\n\n";

    for (const auto& ki : _instructions)
    {

        const MaterialMaster* mat =
            MaterialRegistry::instance().findById(ki.materialId);

        const Cutting::Plan::Request* req =
            CuttingPlanRequestRegistry::instance().findById(ki.requestId);

        out << "Step " << ki.globalStepId << "\n";
        out << "  RequestId: " << req->toString() << "\n";
        out << "  MaterialId: " << mat->toReportLabel() << "\n";
        out << "  Role: " << ki.role.barcodePrefix << "\n";
        out << "  ExternalRef: " << ki.externalReference << "\n";

        out << "  Attributes:\n";
        for (auto it = ki.attributes.begin(); it != ki.attributes.end(); ++it)
            out << "    - " << it.key() << ": " << it.value() << "\n";

        if (ki.fallbackUsed)
            out << "  FallbackUsed: true\n";

        if (ki.alternativeUsed)
            out << "  AlternativeUsed: true\n";

        out << "\n";
    }

    zEvent(QString("📄 KittingInstructions exportálva: %1").arg(path));
}

