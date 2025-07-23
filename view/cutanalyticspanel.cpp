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

void CutAnalyticsPanel::updateStats(const QVector<CutPlan>& plans, const QVector<CutResult>& leftovers)
{
    int totalCuts = 0;
    int totalKerf = 0;
    int totalWaste = 0;
    int segmentCount = 0;
    int pieceCount = 0;
    int kerfCount = 0;
    int wasteCount = 0;

    for (const CutPlan& plan : plans) {
        totalCuts += plan.cuts.size();
        totalKerf += plan.kerfTotal;
        totalWaste += plan.waste;

        segmentCount += plan.segments.size();
        for (const Segment& s : plan.segments) {
            switch (s.type) {
            case SegmentType::Piece: pieceCount++; break;
            case SegmentType::Kerf:  kerfCount++;  break;
            case SegmentType::Waste: wasteCount++; break;
            }
        }
    }

    int reusableWasteCount = std::count_if(leftovers.begin(), leftovers.end(), [](const CutResult& r) {
        return r.waste >= 300;
    });

    int finalWasteCount = std::count_if(leftovers.begin(), leftovers.end(), [](const CutResult& r) {
        return r.isFinalWaste;
    });

    // double efficiency = plans.isEmpty() ? 0.0 :
    //                         static_cast<double>(pieceCount * 1000 - totalKerf - totalWaste) /
    //                             static_cast<double>(pieceCount * 1000) * 100.0;

    int totalPieceLength = 0;
    for (const CutPlan& plan : plans) {
        for (int len : plan.cuts)
            totalPieceLength += len;
    }

    double efficiency = (totalPieceLength * 1.0) / (totalPieceLength + totalKerf + totalWaste) * 100.0;

    // 🧾 Összesített szövegek
    lblSummary->setText(QString("📊 Darabolás: %1 darab, %2 kerf (%3 mm), %4 hulladék (%5 mm)")
                            .arg(pieceCount)
                            .arg(kerfCount)
                            .arg(totalKerf)
                            .arg(wasteCount)
                            .arg(totalWaste));

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
