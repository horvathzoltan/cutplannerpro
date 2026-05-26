#pragma once

#include <QUuid>
#include <QString>

#include <model/cutting/plan/parentinfo.h>

enum class RodOrigin { Stock, Reusable, Continuation };

struct SelectedRod {
    QUuid materialId;
    int length = 0;
    bool isReusable = false;
    QString barcode;   // külső címke (RST-xxxx)
    QString rodId;     // belső identitás (ROD-xxxx)
    std::optional<QUuid> entryId; // csak ha reusable

    RodOrigin origin = RodOrigin::Stock;
    std::optional<Cutting::Plan::ParentInfo> _parent = std::nullopt;
};
