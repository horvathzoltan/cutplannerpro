#include "resultmodel.h"

#include "../../registries/materialregistry.h"
#include "../../../common/materialutils.h"

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
    case Source::FromStock:
        return optimizationId.has_value()
                   ? QString("Stock:Op%1").arg(*optimizationId)
                   : "Stock";
    case Source::FromReusable:
        return "Reusable";
    case Source::Unknown:
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
