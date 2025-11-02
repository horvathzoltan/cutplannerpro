#pragma once

#include "common/logger.h"
#include <QString>
#include <QUuid>

#include <common/identifierutils.h>
#include <common/settingsmanager.h>

/**
 * @brief Darabolási szakasz típusa — a rúd struktúrájához
 */

namespace Cutting{
namespace Segment{

struct SegmentModel {
public:
    /**
 * @brief Egy szakasz a vágási tervben (darab, kerf, hulladék)
 */

    enum class Type {
        Piece,   // ✂️ Kért darab
        Kerf,    // ⚙️ Vágási veszteség
        Waste    // 🪓 Végmaradék / selejt
    };

public:
    QUuid _segId;
    Type _type;
    double _length_mm;
    QString _barcode;
    int _segIx;

    SegmentModel(Type t, double len, int ix):
        _segId(QUuid::createUuid()),
        _type(t),
        _length_mm(len),
        _segIx(ix)
    {
        switch(t){
        case Type::Piece:{
                int matId = SettingsManager::instance().nextMaterialCounter();
                _barcode = IdentifierUtils::makeMaterialId(matId);
                break;
        }
            case Type::Kerf:
                _barcode = "KERF";
                break;
            case Type::Waste:{
                int wasteId = SettingsManager::instance().nextLeftoverCounter();
                _barcode = IdentifierUtils::makeLeftoverId(wasteId);
                break;
            }
        }
    }

    Type type() const { return _type;}
    QString length_txt() const { return QString::number(_length_mm)+ "mm";}
    double length_mm() const { return _length_mm; }
    QString barcode() const { return _barcode; }

    static QString segmentPrefix(SegmentModel::Type type) {
        if(type == SegmentModel::Type::Piece) return "P";
        if(type == SegmentModel::Type::Waste) return "W";
        if(type == SegmentModel::Type::Kerf)  return "K";
        return "?";
    }


    /**
     * @brief Rövid string a munkalaphoz (pl. [1800], [K3], [W194])
     */
    QString toLabelString() const {
        QString prefix = segmentPrefix(_type);

        QString postfix = (_type == Type::Kerf)
                              ? QString::number(_segIx)
                              : QString("%1:%2·%3mm")
                                    .arg(_segIx)
                                    .arg(_barcode.isEmpty() ? "∅" : _barcode)
                                    .arg(_length_mm, 0, 'f', 0);

        return prefix+postfix;
    }

    //QVector<SegmentModel> generateSegments(double kerf_mm, double totalLength_mm) const;
};
}} //end namespace Cutting::Segment
