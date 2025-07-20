#include "cuttingrequest.h"

//CuttingRequest::CuttingRequest() {}

bool CuttingRequest::isValid() const {
    if(materialId.isNull()) return false;
    if(requiredLength <= 0) return false;
    if(quantity <= 0) return false;
    return true;
}

QString CuttingRequest::invalidReason() const {
    if (materialId.isNull())
        return "Anyag ID hiányzik vagy érvénytelen.";

    if (requiredLength <= 0)
        return "A vágáshossz nem lehet nulla vagy negatív.";

    if (quantity <= 0)
        return "A darabszám nem lehet nulla vagy negatív.";

    return {}; // Érvényes esetben üres string
}
