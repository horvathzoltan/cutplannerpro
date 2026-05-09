#pragma once

#include <QUuid>
#include <QString>

struct SelectedRod {
    QUuid materialId;
    int length = 0;
    bool isReusable = false;
    QString barcode;   // külső címke (RST-xxxx)
    QString rodId;     // belső identitás (ROD-xxxx)
    std::optional<QUuid> entryId; // csak ha reusable
};
