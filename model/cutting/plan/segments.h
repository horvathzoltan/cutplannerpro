#pragma once

#include <model/cutting/segment/segmentmodel.h>
#include <model/cutting/piece/piecewithmaterial.h>
#include "../../../service/cutting/segment/segmentutils.h"


namespace Cutting {
namespace Plan {

class Segments{
    //Segments() = delete;

public:
    struct SegnemntInfo {
        double length = 0;
        int count = 0;
    };

private:
    QVector<Cutting::Segment::SegmentModel> _segments; // 🧱 Vágási szakaszok — vizuális és logikai bontás
    QString _leftoverBarcode; // ♻️ Ha a rúd végén leftover keletkezik, itt tároljuk az új barcode-ot

    int _totalLength_mm = 0;      // 📏 Anyag hossz (mm)

    SegnemntInfo getSegmentInfo(Cutting::Segment::SegmentModel::Type smt)const {
        SegnemntInfo si;
        for (const auto& s : _segments) {
            if (s._type == smt) {
                si.length += s.length_mm();
                si.count++;
            }
        }
        return si;
    }

public:

    const QVector<Cutting::Segment::SegmentModel>& segments() const {return _segments;}

    bool isEmpty(){return _segments.isEmpty();}

    int totalLength_mm() const { return _totalLength_mm;}
    double totalLength_m() const { return _totalLength_mm / 1000;}
    void SetTotalLength_mm(int x){ _totalLength_mm = x; }

    double waste_mm() const {
        double used = 0;
        for (auto& s : _segments)
            used += s.length_mm();
        double w = _totalLength_mm - used;

        // ha kerfre végződik, és a maradék kisebb mint a kerf → a fűrész megeszi
        if (!_segments.isEmpty()) {
            const auto& last = _segments.last();
            if (last.isKerf()) {
                double kerf = last.length_mm();

                // ha a maradék a kerf tartományába esik (pozitív vagy negatív)
                if (w <= 0.0 && w >= -kerf)
                    return 0.0;

                if (w > 0.0 && w <= kerf)
                    return 0.0;
            }
        }

        return w;
    }

    int size() const { return _segments.size(); }

    void clear() { _segments.clear(); }

    SegnemntInfo piecesInfo() const {
        return getSegmentInfo(Cutting::Segment::SegmentModel::Type::Piece);
    }

    SegnemntInfo kerfInfo() const {
        return getSegmentInfo(Cutting::Segment::SegmentModel::Type::Kerf);
    }

    SegnemntInfo technicalInfo() const {
        return getSegmentInfo(Cutting::Segment::SegmentModel::Type::Technical);
    }

    void insert(int ix, Cutting::Segment::SegmentModel s){ _segments.insert(ix, s); }

    QString leftoverBarcode() const { return _leftoverBarcode; }

    const Cutting::Segment::SegmentModel segment(int i) const {return _segments[i];}

    void setSegments(QVector<Segment::SegmentModel> s1){ _segments = s1; }

    //void setLeftoverBarcode(const QString& bc){ _leftoverBarcode = bc; }

    void generateSegments(const QVector<Cutting::Piece::PieceWithMaterial>& cuts,
                          double kerf_mm, int totalLength_mm){
        auto seg1 = Cutting::Segment::SegmentUtils::generateSegments(
            cuts, kerf_mm, totalLength_mm);

        setSegments(seg1);
        SetTotalLength_mm(totalLength_mm);

        // ♻️ leftover barcode generálás
        double w = waste_mm();
        if (w >= 300) {
            int id = SettingsManager::instance().nextLeftoverCounter();
            auto barcode = IdentifierUtils::makeLeftoverId(id);
            //setLeftoverBarcode(barcode);
            _leftoverBarcode = barcode;
        } else{
            _leftoverBarcode = "selejt";
        }
    }
};

} // endof namespace Plan
} // endof namespace Cutting