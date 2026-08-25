#pragma once

#include <QStringList>

struct ValidationResult{
    QStringList errors;
    QStringList warnings;
    QStringList informations;

    bool hasError() const {return !errors.isEmpty();}
    bool hasWarning() const {return !warnings.isEmpty();}
    bool hasInfo() const {return !informations.isEmpty();}
};