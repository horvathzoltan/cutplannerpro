#pragma once
#include "calculation/sizecalcmode.h"
#include <QString>
#include <QVector>

struct ProductCalcModes {
    QString typeCode;
    QString subtypeCode;

    QVector<SizeCalcMode> modes;
    SizeCalcMode defaultMode = SizeCalcMode::Gyartasi;
};
