#pragma once

#include <QVector>
#include "model/cutting/plan/request.h"

enum class NaphaloType {
    Unknown,
    Cipzaros,
    Sines,
    Bowdenes
};

class NaphaloTypeDetector {
public:
    static NaphaloType detect(const QVector<Cutting::Plan::Request>& list);
};
