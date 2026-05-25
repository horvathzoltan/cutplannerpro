#include "cutanalyticspanel.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include <numeric>

CutAnalyticsPanel::CutAnalyticsPanel(QWidget* parent)
    : QFrame(parent)
{
    // 📐 Keretstílus, de háttér szín a palettából (nem fix)
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Window); // örökölt háttér — sötét, világos, stb.

    setupLayout();
}

void CutAnalyticsPanel::setupLayout()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    lblSummary     = new QLabel(this);
    lblSegments    = new QLabel(this);
    lblFinalWaste  = new QLabel(this);
    lblEfficiency  = new QLabel(this);

    // 🖼️ A program által használt alap font, picit kiemelve
    QFont baseFont = font();
    baseFont.setBold(true);
    baseFont.setPointSize(baseFont.pointSize() + 1);

    for (QLabel* lbl : { lblSummary, lblSegments, lblFinalWaste, lblEfficiency }) {
        lbl->setFont(baseFont);
        lbl->setWordWrap(true); // ha hosszabb a szöveg → ne vágódjon le
        layout->addWidget(lbl);
    }
}


void CutAnalyticsPanel::updateStats(const QVector<Cutting::Plan::CutPlan>& plans, const QVector<Cutting::Result::ResultModel>& leftovers)
{
    // 🔢 Inicializálás
    int totalCuts          = 0;
    int segmentCount       = 0;
    int pieceCount         = 0;
    int kerfCount          = 0;
    int wasteCount         = 0;
    int totalPieceLength   = 0;
    int totalKerfLength    = 0;
    int totalWasteLength   = 0;

    // 📊 Tervek bejárása
    for (const Cutting::Plan::CutPlan& plan : plans) {
        totalCuts += plan.piecesWithMaterial.size();          // vágások száma
        segmentCount += plan._segments.size();   // teljes szakaszszám

        auto pieceInfo = plan._segments.piecesInfo();
        pieceCount = pieceInfo.count;
        totalPieceLength += pieceInfo.length;
        auto kerfInfo = plan._segments.kerfInfo();
        kerfCount = kerfInfo.count;
        totalKerfLength += kerfInfo.length;
        totalWasteLength = plan._segments.waste_mm();

        // for (const Cutting::Segment::SegmentModel& s : plan.segments) {
        //     if(s.isPiece()){
        //         pieceCount++;
        //         totalPieceLength += s.length_mm();
        //     }else if(s.isKerf()){
        //         kerfCount++;
        //         totalKerfLength += s.length_mm();
        //     } else if(s.isWaste()){
        //         wasteCount++;
        //         totalWasteLength += s.length_mm();
        //     }
        // }
    }

    // ♻️ Újrahasznosítható maradékok száma (min. 300mm)
    int reusableWasteCount = std::count_if(leftovers.begin(), leftovers.end(), [](const Cutting::Result::ResultModel& r) {
        return r.waste >= 300;
    });

    // 🗃️ Végleges hulladékok száma
    int finalWasteCount = std::count_if(leftovers.begin(), leftovers.end(), [](const Cutting::Result::ResultModel& r) {
        return r.isFinalWaste;
    });

    // 🚦 Hatékonyság: darabok / (darab + kerf + hulladék)
    double efficiency = (totalPieceLength == 0) ? 0.0
                                                : static_cast<double>(totalPieceLength) /
                                                      static_cast<double>(totalPieceLength + totalKerfLength + totalWasteLength) * 100.0;

    // 🖥️ GUI címkék frissítése
    lblSummary->setText(QString("📊 Darabolás: %1 darab, %2 kerf (%3 mm), %4 hulladék (%5 mm)")
                            .arg(pieceCount)
                            .arg(kerfCount)
                            .arg(totalKerfLength)
                            .arg(wasteCount)
                            .arg(totalWasteLength));

    lblSegments->setText(QString("📐 Szakaszok összesen: %1 (%2 darab + %3 kerf + %4 hulladék)")
                             .arg(segmentCount)
                             .arg(pieceCount)
                             .arg(kerfCount)
                             .arg(wasteCount));

    lblFinalWaste->setText(QString("♻️ Újrahasználható: %1 db • Archivált végmaradék: %2 db")
                               .arg(reusableWasteCount)
                               .arg(finalWasteCount));

    lblEfficiency->setText(QString("🚦 Hatékonysági mutató: %1%")
                               .arg(QString::number(efficiency, 'f', 1)));
}

