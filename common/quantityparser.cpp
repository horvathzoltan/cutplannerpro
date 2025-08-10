// quantityparser.cpp
#include "quantityparser.h"
#include <QRegularExpression>

QuantityParser::Mode QuantityParser::parse(const QString& input, int& outValue) {
    QString trimmed = input.trimmed();

    static const QRegularExpression re("^\\s*([+-])\\s*(\\d+)\\s*$");

    if (trimmed.startsWith("+") || trimmed.startsWith("-")) {
        QRegularExpressionMatch match = re.match(trimmed);

        if (match.hasMatch()) {
            QString op = match.captured(1);
            int delta = match.captured(2).toInt();

            outValue = (op == "+") ? delta : -delta;
            return Relative;
        }
        return Invalid;
    }

    bool ok = false;
    int value = trimmed.toInt(&ok);
    if (ok) {
        outValue = value;
        return Absolute;
    }

    return Invalid;
}
