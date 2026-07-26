#pragma once
#include <QStringList>

struct Tolerance{
    double min_mm; ///< negatív eltérés mm-ben
    double max_mm; ///< pozitív eltérés mm-ben

    // CSV-hez: csak a tűrés szintaxis
    QString toCsvString(bool withUnit = false) const {
        QString unit = withUnit ? " mm" : "";
        if (qFuzzyCompare(std::abs(min_mm), std::abs(max_mm)) && min_mm < 0 && max_mm > 0) {
            return QString("+/-%1%2").arg(max_mm).arg(unit);
        }
        return QString("%1/%2%3").arg(min_mm).arg(max_mm).arg(unit);
    }

    // Emberi olvasásra: nominális mérettel kombinálva
    QString toString(double nominal = 0, bool withUnit = true) const {
        QString unit = withUnit ? " mm" : "";
        if (qFuzzyCompare(std::abs(min_mm), std::abs(max_mm)) && min_mm < 0 && max_mm > 0) {
            if (nominal != 0) {
                return QString("%1 ±%2%3").arg(nominal).arg(max_mm).arg(unit);
            }
            return QString("+/-%1%2").arg(max_mm).arg(unit);
        }
        if (nominal != 0) {
            return QString("%1 %2/%3%4").arg(nominal).arg(min_mm).arg(max_mm).arg(unit);
        }
        return QString("%1/%2%3").arg(min_mm).arg(max_mm).arg(unit);
    }

    static std::optional<Tolerance> fromString(const QString& s) {
        if (s.isEmpty()) return std::nullopt;
        QString str = s.trimmed();

        if (str.startsWith("+/-")) {
            bool ok = false;
            double val = str.mid(3).toDouble(&ok);
            if (ok) return Tolerance{ -val, val };
            return std::nullopt;
        }

        if (str.contains("±")) {
            auto parts = str.split("±");
            if (parts.size() == 2) {
                bool ok = false;
                double val = parts[1].remove("mm").trimmed().toDouble(&ok);
                if (ok) return Tolerance{ -val, val };
            }
        }

        auto parts = str.split('/');
        if (parts.size() == 2) {
            bool ok1 = false, ok2 = false;
            double minVal = parts[0].remove("mm").trimmed().toDouble(&ok1);
            double maxVal = parts[1].remove("mm").trimmed().toDouble(&ok2);
            if (ok1 && ok2) return Tolerance{ minVal, maxVal };
        }

        return std::nullopt;
    }
};
