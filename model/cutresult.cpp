#include "cutresult.h"

#include "materialregistry.h"

QString CutResult::cutsAsString() const {
    QStringList list;
    for (int c : cuts)
        list << QString::number(c);
    return list.join(" + ");
}

QString CutResult::sourceAsString() const {
    if (source == LeftoverSource::Manual)
        return "Manuális";
    if (source == LeftoverSource::Optimization)
        return optimizationId.has_value()
                   ? QString("Op:%1").arg(*optimizationId)
                   : "Op:?";
    return "Ismeretlen";
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
    return m ? CategoryUtils::badgeColorForCategory(m->category) : QColor(Qt::gray);
}

