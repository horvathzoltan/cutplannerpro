#include "cutresult.h"

#include "registries/materialregistry.h"
#include "../common/materialutils.h"

QString CutResult::cutsAsString() const {
    QStringList list;
    for (int c : cuts)
        list << QString::number(c);
    return list.join(" + ");
}

QString CutResult::sourceAsString() const {
    switch (source) {
    case CutResultSource::FromStock:
        return optimizationId.has_value()
                   ? QString("Stock:Op%1").arg(*optimizationId)
                   : "Stock";
    case CutResultSource::FromReusable:
        return "Reusable";
    case CutResultSource::Unknown:
    default:
        return "Ismeretlen";
    }
}

QString CutResult::materialName() const {
    const auto& m = MaterialRegistry::instance().findById(materialId);
    return m? m->name : "(ismeretlen)";
}

MaterialType CutResult::materialType() const {
    const auto& m = MaterialRegistry::instance().findById(materialId);
    return m ? m->type : MaterialType(MaterialType::Type::Unknown);
}

QColor CutResult::categoryColor() const {
    const auto& m = MaterialRegistry::instance().findById(materialId);
    return m ? MaterialUtils::colorForMaterial(*m) : QColor(Qt::gray);
}

