#include "resultmodel.h"

#include "../../registries/materialregistry.h"
#include "model/material/material_utils.h"

namespace Cutting{
namespace Result{

QString ResultModel::cutsAsString() const {
    QStringList list;
    for (const Cutting::Piece::PieceWithMaterial& piece : cuts) {
        list << QString::number(piece.info.length_mm);
    }
    return list.join(";");
}





QString ResultModel::sourceAsString() const {
    switch (source) {
    case ResultSource::FromStock:
        return optimizationId.has_value()
                   ? QString("Stock:Op%1").arg(*optimizationId)
                   : "Stock";
    case ResultSource::FromReusable:
        return "Reusable";
    case ResultSource::Unknown:
    default:
        return "Ismeretlen";
    }
}

QString ResultModel::materialName() const {
    const auto& m = MaterialRegistry::instance().findById(materialId);
    return m? m->name : "(ismeretlen)";
}

MaterialType ResultModel::materialType() const {
    const auto& m = MaterialRegistry::instance().findById(materialId);
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}

QColor ResultModel::materialGroupColor() const {
    const auto& m = MaterialRegistry::instance().findById(materialId);
    return m ? MaterialUtils::colorForMaterial(*m) : QColor(Qt::gray);
}
}} //end of namespace Cutting::Result
