#pragma once

#include <QUuid>
#include <QString>

namespace Cutting {
namespace Plan {

struct ParentInfo{
private:
    // Annak az entitásnak az azonosítója, amelyből ez a CutPlan készült.
    // Lehet:
    //  • stock rúd mesterséges kódja (MAT-xxxx)
    //  • leftover kódja (RST-xxxx)
    //  • előző CutPlan sourceBarcode-ja (folytatás esetén)
    QString _barcode;

    // Annak a CutPlan-nek az azonosítója, amely létrehozta a leftovert,
    // vagy folytatás esetén az előző CutPlan azonosítója.
    // Stock esetén nincs értéke.
    std::optional<QUuid> _planId;

public:
    ParentInfo(QString barcode, std::optional<QUuid> planId)
        : _barcode(std::move(barcode)), _planId(std::move(planId)) {}

    const QString& barcode() const { return _barcode; }
    const std::optional<QUuid>& planId() const { return _planId; }

    QString toString() const {
        return _planId.has_value()
        ? QString("%1 (from plan %2)").arg(_barcode, _planId->toString())
        : QString("%1 (stock)").arg(_barcode);
    }


    QString toCsv() const
    {
        QString pid = _planId.has_value() ? _planId->toString() : "";
        return QString("%1;%2").arg(_barcode, pid);
    }

    static ParentInfo fromCsv(const QString& barcode, const QString& planIdStr)
    {
        std::optional<QUuid> pid;
        if (!planIdStr.isEmpty())
            pid = QUuid(planIdStr);

        return ParentInfo(barcode, pid);
    }


};

} //namespace Plan
}  //namespace Cutting