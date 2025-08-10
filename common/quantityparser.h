// quantityparser.h
#ifndef QUANTITYPARSER_H
#define QUANTITYPARSER_H

#include <QString>

class QuantityParser {
public:
    enum Mode {
        Invalid = -1,
        Absolute = 0,
        Relative = 1
    };

    static Mode parse(const QString& input, int& outValue);
};

#endif // QUANTITYPARSER_H
