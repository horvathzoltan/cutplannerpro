#pragma once

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUuid>
#include <QVariant>

#include <optional>

class RowTracker2
{
public:

    static constexpr int IdRole =
        Qt::UserRole;

    static constexpr int ExtraRole =
        Qt::UserRole + 1;

    explicit RowTracker2(
        QTableWidget* table);

    static QTableWidgetItem*
    ensureHeader(
        QTableWidget* table,
        int row);

    void registerRow(
        int row,
        const QUuid& id,
        const QVariant& extra = {});

    std::optional<QUuid>
    idOfRow(
        int row) const;

    std::optional<int>
    rowOf(
        const QUuid& id) const;

    std::optional<QVariant>
    extraOfRow(
        int row) const;

    std::optional<QVariant>
    extraOfId(
        const QUuid& id) const;

    bool contains(
        const QUuid& id) const;

private:

    QTableWidget* _table = nullptr;
};