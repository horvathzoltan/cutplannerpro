#pragma once

#include <QTableWidget>
#include <optional>
#include "model/cuttingrequest.h"

class InputTableManager {
public:
    explicit InputTableManager(QTableWidget* table, QWidget* parent = nullptr);

    void addRow(const CuttingRequest& request);
    std::optional<CuttingRequest> readRow(int row) const;
    QVector<CuttingRequest> readAll() const;
    void fillTestData();

private:
    QTableWidget* table;
    QWidget* parent;
};
