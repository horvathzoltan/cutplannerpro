#include "cutresult.h"

//CutResult::CutResult() {}

QString CutResult::cutsAsString() const {
    QStringList list;
    for (int cut : cuts)
        list << QString::number(cut);
    return list.join(", ");
}
