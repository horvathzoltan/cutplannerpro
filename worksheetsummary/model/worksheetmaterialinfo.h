#pragma once

#include "meta/meta.h"
#include <QUuid>


// 1 rekord = 1 konkrét levágott anyag
// egy darab konkrét munkadarab reprezentációja annak anyagával és hosszával
struct WorkSheetMaterialInfo
{
    QUuid materialId;
    int cutLengthMm = 0;

private:
    static MetaData<WorkSheetMaterialInfo> _meta;

public:
    static MetaData<WorkSheetMaterialInfo>& Meta();
    static void MetaInit();
};