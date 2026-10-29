#pragma once

#include <QUuid>


// 1 rekord = 1 konkrét levágott anyag
// egy darab konkrét munkadarab reprezentációja annak anyagával és hosszával
struct WorkSheetMaterialInfo
{
    QUuid materialId;
    int cutLengthMm = 0;
};